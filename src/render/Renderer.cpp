#include "render/Renderer.hpp"

#include "core/Log.hpp"
#include "vk/Pipeline.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <cstring>
#include <filesystem>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace velora {

const std::vector<StyleInfo>& styleList() {
  static const std::vector<StyleInfo> styles = {
      {"toon", "toon.vert", "toon.frag",
       "cel shading: banded diffuse, stepped specular, rim light"},
      {"gooch", "toon.vert", "gooch.frag",
       "warm/cool Gooch shading, technical-illustration look"},
      {"hatch", "toon.vert", "hatch.frag",
       "screen-space cross-hatching driven by diffuse light"},
      {"halftone", "toon.vert", "halftone.frag",
       "print-style halftone dot screening"},
      {"ink", "toon.vert", "ink.frag",
       "posterized sumi-e ink wash with dithered edges"},
  };
  return styles;
}

Renderer::Renderer(VulkanContext& ctx, uint32_t width, uint32_t height, int ssaa,
                   std::string shaderDir)
    : ctx_(ctx),
      outW_(width),
      outH_(height),
      rw_(width * ssaa),
      rh_(height * ssaa),
      ssaa_(ssaa),
      shaderDir_(std::move(shaderDir)) {
  meshes_.init(&ctx);
  createRenderPass();
  createTargets();
  createDescriptors();
  createPipelines();

  VkCommandBufferAllocateInfo cai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  cai.commandPool = ctx_.cmdPool;
  cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cai.commandBufferCount = 1;
  VK_CHECK(vkAllocateCommandBuffers(ctx_.device, &cai, &cmd_));

  log::info("renderer ready: %ux%u (internal %ux%u, %dx SSAA)", outW_, outH_, rw_, rh_,
            ssaa_);
}

Renderer::~Renderer() {
  vkDeviceWaitIdle(ctx_.device);

  meshes_.shutdown();

  for (auto& [name, pipe] : stylePipelines_)
    vkDestroyPipeline(ctx_.device, pipe, nullptr);
  if (outlinePipeline_ != VK_NULL_HANDLE)
    vkDestroyPipeline(ctx_.device, outlinePipeline_, nullptr);
  vkDestroyPipelineLayout(ctx_.device, pipelineLayout_, nullptr);

  vkDestroyDescriptorPool(ctx_.device, descriptorPool_, nullptr);
  vkDestroyDescriptorSetLayout(ctx_.device, setLayout_, nullptr);

  unmapBuffer(ctx_, staging_);
  unmapBuffer(ctx_, ubo_);
  destroyBuffer(ctx_, staging_);
  destroyBuffer(ctx_, ubo_);

  vkDestroyFramebuffer(ctx_.device, framebuffer_, nullptr);
  destroyImage(ctx_, color_);
  destroyImage(ctx_, depth_);
  vkDestroyRenderPass(ctx_.device, renderPass_, nullptr);
}

void Renderer::createRenderPass() {
  VkAttachmentDescription color{};
  color.format = VK_FORMAT_R8G8B8A8_UNORM;
  color.samples = VK_SAMPLE_COUNT_1_BIT;
  color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

  VkAttachmentDescription depth{};
  depth.format = VK_FORMAT_D32_SFLOAT;
  depth.samples = VK_SAMPLE_COUNT_1_BIT;
  depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  VkAttachmentReference depthRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorRef;
  subpass.pDepthStencilAttachment = &depthRef;

  VkSubpassDependency dep{};
  dep.srcSubpass = VK_SUBPASS_EXTERNAL;
  dep.dstSubpass = 0;
  dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                     VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                     VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  VkAttachmentDescription attachments[2] = {color, depth};
  VkRenderPassCreateInfo rpci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  rpci.attachmentCount = 2;
  rpci.pAttachments = attachments;
  rpci.subpassCount = 1;
  rpci.pSubpasses = &subpass;
  rpci.dependencyCount = 1;
  rpci.pDependencies = &dep;
  VK_CHECK(vkCreateRenderPass(ctx_.device, &rpci, nullptr, &renderPass_));
}

void Renderer::createTargets() {
  color_ = createImage2D(ctx_, rw_, rh_, VK_FORMAT_R8G8B8A8_UNORM,
                         VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                         VK_IMAGE_ASPECT_COLOR_BIT);
  depth_ = createImage2D(ctx_, rw_, rh_, VK_FORMAT_D32_SFLOAT,
                         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                         VK_IMAGE_ASPECT_DEPTH_BIT);

  VkImageView views[2] = {color_.view, depth_.view};
  VkFramebufferCreateInfo fci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
  fci.renderPass = renderPass_;
  fci.attachmentCount = 2;
  fci.pAttachments = views;
  fci.width = rw_;
  fci.height = rh_;
  fci.layers = 1;
  VK_CHECK(vkCreateFramebuffer(ctx_.device, &fci, nullptr, &framebuffer_));

  staging_ = createBuffer(ctx_, (VkDeviceSize)rw_ * rh_ * 4,
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  stagingMapped_ = mapBuffer(ctx_, staging_);

  ubo_ = createBuffer(ctx_, sizeof(FrameUbo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  uboMapped_ = mapBuffer(ctx_, ubo_);
}

void Renderer::createDescriptors() {
  VkDescriptorSetLayoutBinding binding{};
  binding.binding = 0;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  binding.descriptorCount = 1;
  binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo lci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  lci.bindingCount = 1;
  lci.pBindings = &binding;
  VK_CHECK(vkCreateDescriptorSetLayout(ctx_.device, &lci, nullptr, &setLayout_));

  VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1};
  VkDescriptorPoolCreateInfo pci{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  pci.maxSets = 1;
  pci.poolSizeCount = 1;
  pci.pPoolSizes = &poolSize;
  VK_CHECK(vkCreateDescriptorPool(ctx_.device, &pci, nullptr, &descriptorPool_));

  VkDescriptorSetAllocateInfo sai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  sai.descriptorPool = descriptorPool_;
  sai.descriptorSetCount = 1;
  sai.pSetLayouts = &setLayout_;
  VK_CHECK(vkAllocateDescriptorSets(ctx_.device, &sai, &descriptorSet_));

  VkDescriptorBufferInfo dbi{ubo_.buffer, 0, sizeof(FrameUbo)};
  VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  write.dstSet = descriptorSet_;
  write.dstBinding = 0;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  write.pBufferInfo = &dbi;
  vkUpdateDescriptorSets(ctx_.device, 1, &write, 0, nullptr);

  VkPushConstantRange pushRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                0, sizeof(PushBlock)};
  VkPipelineLayoutCreateInfo plci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  plci.setLayoutCount = 1;
  plci.pSetLayouts = &setLayout_;
  plci.pushConstantRangeCount = 1;
  plci.pPushConstantRanges = &pushRange;
  VK_CHECK(vkCreatePipelineLayout(ctx_.device, &plci, nullptr, &pipelineLayout_));
}

void Renderer::createPipelines() {
  for (const auto& style : styleList()) {
    GraphicsPipelineDesc desc;
    desc.vertShader = shaderDir_ + "/" + style.vertShader;
    desc.fragShader = shaderDir_ + "/" + style.fragShader;
    desc.cullMode = VK_CULL_MODE_BACK_BIT;
    stylePipelines_[style.name] =
        createGraphicsPipeline(ctx_, renderPass_, pipelineLayout_, desc);
    log::info("built style pipeline: %s", style.name.c_str());
  }

  GraphicsPipelineDesc outline;
  outline.vertShader = shaderDir_ + "/outline.vert";
  outline.fragShader = shaderDir_ + "/outline.frag";
  outline.cullMode = VK_CULL_MODE_FRONT_BIT; // inverted hull: draw back faces
  outlinePipeline_ = createGraphicsPipeline(ctx_, renderPass_, pipelineLayout_, outline);
  log::info("built outline pipeline (inverted hull)");
}

PushBlock Renderer::makePush(const Node& node) const {
  glm::mat4 m(1.0f);
  m = glm::translate(m, node.position);
  if (node.rotationDeg != 0.0f)
    m = glm::rotate(m, glm::radians(node.rotationDeg), node.rotationAxis);
  m = glm::scale(m, glm::vec3(node.scale));

  const Material& mat = node.material;
  PushBlock p{};
  p.model = m;
  p.colorA = glm::vec4(mat.baseColor, mat.bands);
  p.colorB = glm::vec4(mat.specColor, mat.shininess);
  p.params = glm::vec4(mat.rimStrength, mat.rimPower, mat.outlineWidth, 0.0f);
  p.outlineColor = glm::vec4(mat.outlineColor, 1.0f);
  return p;
}

void Renderer::drawNode(const Node& node) {
  const GpuMesh& mesh = meshes_.get(node.mesh);
  PushBlock push = makePush(node);
  vkCmdPushConstants(cmd_, pipelineLayout_,
                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                     sizeof(PushBlock), &push);
  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(cmd_, 0, 1, &mesh.vertexBuffer.buffer, &offset);
  vkCmdBindIndexBuffer(cmd_, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(cmd_, mesh.indexCount, 1, 0, 0, 0);
}

void Renderer::render(const Scene& scene, const std::string& style,
                      float azimuthOffsetDeg, const std::string& outPath) {
  auto pipeIt = stylePipelines_.find(style);
  if (pipeIt == stylePipelines_.end())
    log::fatal("unknown style: " + style);

  auto t0 = std::chrono::steady_clock::now();

  Camera cam = scene.camera;
  cam.azimuthDeg += azimuthOffsetDeg;

  FrameUbo ubo{};
  ubo.view = cam.view();
  ubo.proj = cam.proj((float)rw_ / (float)rh_);
  ubo.lightDir = glm::vec4(glm::normalize(scene.lightDir), 0.0f);
  ubo.cameraPos = glm::vec4(cam.eye(), 1.0f);
  ubo.screen = glm::vec4((float)rw_, (float)rh_, 0.0f, 0.0f);
  std::memcpy(uboMapped_, &ubo, sizeof(ubo));

  VK_CHECK(vkResetCommandBuffer(cmd_, 0));
  VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  VK_CHECK(vkBeginCommandBuffer(cmd_, &bi));

  VkClearValue clears[2];
  clears[0].color = {scene.clearColor.r, scene.clearColor.g, scene.clearColor.b,
                     scene.clearColor.a};
  clears[1].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo rpbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  rpbi.renderPass = renderPass_;
  rpbi.framebuffer = framebuffer_;
  rpbi.renderArea = {{0, 0}, {rw_, rh_}};
  rpbi.clearValueCount = 2;
  rpbi.pClearValues = clears;
  vkCmdBeginRenderPass(cmd_, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{0.0f, 0.0f, (float)rw_, (float)rh_, 0.0f, 1.0f};
  VkRect2D scissor{{0, 0}, {rw_, rh_}};
  vkCmdSetViewport(cmd_, 0, 1, &viewport);
  vkCmdSetScissor(cmd_, 0, 1, &scissor);
  vkCmdBindDescriptorSets(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_, 0, 1,
                          &descriptorSet_, 0, nullptr);

  // Pass 1: ink outlines (inverted hull).
  vkCmdBindPipeline(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, outlinePipeline_);
  for (const Node& node : scene.nodes) {
    if (node.material.outlineWidth > 0.0f)
      drawNode(node);
  }

  // Pass 2: stylized fill.
  vkCmdBindPipeline(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeIt->second);
  for (const Node& node : scene.nodes)
    drawNode(node);

  vkCmdEndRenderPass(cmd_);

  // Render pass ends with the color image in TRANSFER_SRC_OPTIMAL.
  VkBufferImageCopy copy{};
  copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
  copy.imageExtent = {rw_, rh_, 1};
  vkCmdCopyImageToBuffer(cmd_, color_.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                         staging_.buffer, 1, &copy);

  VK_CHECK(vkEndCommandBuffer(cmd_));
  VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  si.commandBufferCount = 1;
  si.pCommandBuffers = &cmd_;
  VK_CHECK(vkQueueSubmit(ctx_.queue, 1, &si, VK_NULL_HANDLE));
  VK_CHECK(vkQueueWaitIdle(ctx_.queue));

  // Readback + box-filter downsample.
  const uint8_t* src = static_cast<const uint8_t*>(stagingMapped_);
  std::vector<uint8_t> out((size_t)outW_ * outH_ * 4);
  for (uint32_t y = 0; y < outH_; y++) {
    for (uint32_t x = 0; x < outW_; x++) {
      for (int c = 0; c < 4; c++) {
        uint32_t sum = 0;
        for (int dy = 0; dy < ssaa_; dy++)
          for (int dx = 0; dx < ssaa_; dx++)
            sum += src[((size_t)(y * ssaa_ + dy) * rw_ + (x * ssaa_ + dx)) * 4 + c];
        out[((size_t)y * outW_ + x) * 4 + c] = (uint8_t)(sum / (ssaa_ * ssaa_));
      }
    }
  }

  std::filesystem::path path(outPath);
  if (path.has_parent_path())
    std::filesystem::create_directories(path.parent_path());
  if (!stbi_write_png(outPath.c_str(), (int)outW_, (int)outH_, 4, out.data(),
                      (int)outW_ * 4))
    log::fatal("failed to write " + outPath);

  auto t1 = std::chrono::steady_clock::now();
  double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
  log::info("rendered %-40s (%ux%u, %.0f ms)", outPath.c_str(), outW_, outH_, ms);
}

} // namespace velora
