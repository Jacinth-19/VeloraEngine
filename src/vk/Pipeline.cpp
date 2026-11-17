#include "vk/Pipeline.hpp"

#include "render/Mesh.hpp"
#include "vk/ShaderLibrary.hpp"

#include <vector>

namespace velora {

static VkShaderModule createModule(VulkanContext& ctx, const std::vector<uint32_t>& spirv) {
  VkShaderModuleCreateInfo mci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  mci.codeSize = spirv.size() * sizeof(uint32_t);
  mci.pCode = spirv.data();
  VkShaderModule mod;
  VK_CHECK(vkCreateShaderModule(ctx.device, &mci, nullptr, &mod));
  return mod;
}

VkPipeline createGraphicsPipeline(VulkanContext& ctx, VkRenderPass renderPass,
                                  VkPipelineLayout layout,
                                  const GraphicsPipelineDesc& desc) {
  auto vsSpirv = compileShaderFile(VK_SHADER_STAGE_VERTEX_BIT, desc.vertShader);
  auto fsSpirv = compileShaderFile(VK_SHADER_STAGE_FRAGMENT_BIT, desc.fragShader);
  VkShaderModule vs = createModule(ctx, vsSpirv);
  VkShaderModule fs = createModule(ctx, fsSpirv);

  VkPipelineShaderStageCreateInfo stages[2]{};
  stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  stages[0].module = vs;
  stages[0].pName = "main";
  stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  stages[1].module = fs;
  stages[1].pName = "main";

  VkVertexInputBindingDescription binding{};
  binding.binding = 0;
  binding.stride = sizeof(Vertex);
  binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputAttributeDescription attrs[2]{};
  attrs[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)};
  attrs[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)};

  VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  vi.vertexBindingDescriptionCount = 1;
  vi.pVertexBindingDescriptions = &binding;
  vi.vertexAttributeDescriptionCount = 2;
  vi.pVertexAttributeDescriptions = attrs;

  VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  vp.viewportCount = 1;
  vp.scissorCount = 1; // dynamic viewport/scissor

  VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  rs.polygonMode = VK_POLYGON_MODE_FILL;
  rs.cullMode = desc.cullMode;
  rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rs.lineWidth = 1.0f;

  VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
  ds.depthTestEnable = VK_TRUE;
  ds.depthWriteEnable = desc.depthWrite;
  ds.depthCompareOp = desc.depthCompare;

  VkPipelineColorBlendAttachmentState blendAtt{};
  blendAtt.blendEnable = VK_FALSE;
  blendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  VkPipelineColorBlendStateCreateInfo cb{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  cb.attachmentCount = 1;
  cb.pAttachments = &blendAtt;

  VkDynamicState dynStates[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dyn{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
  dyn.dynamicStateCount = 2;
  dyn.pDynamicStates = dynStates;

  VkGraphicsPipelineCreateInfo pci{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
  pci.stageCount = 2;
  pci.pStages = stages;
  pci.pVertexInputState = &vi;
  pci.pInputAssemblyState = &ia;
  pci.pViewportState = &vp;
  pci.pRasterizationState = &rs;
  pci.pMultisampleState = &ms;
  pci.pDepthStencilState = &ds;
  pci.pColorBlendState = &cb;
  pci.pDynamicState = &dyn;
  pci.layout = layout;
  pci.renderPass = renderPass;
  pci.subpass = 0;

  VkPipeline pipeline;
  VK_CHECK(vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1, &pci, nullptr, &pipeline));

  vkDestroyShaderModule(ctx.device, vs, nullptr);
  vkDestroyShaderModule(ctx.device, fs, nullptr);
  return pipeline;
}

} // namespace velora
