#pragma once

#include "render/Mesh.hpp"
#include "render/Scene.hpp"
#include "vk/Resources.hpp"
#include "vk/VulkanContext.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace velora {

struct StyleInfo {
  std::string name;
  std::string vertShader;
  std::string fragShader;
  std::string description;
};

const std::vector<StyleInfo>& styleList();

// Uniform block shared by all Velora shaders (set 0, binding 0).
struct FrameUbo {
  glm::mat4 view;
  glm::mat4 proj;
  glm::vec4 lightDir;   // xyz: direction the light travels
  glm::vec4 cameraPos;  // xyz: eye
  glm::vec4 screen;     // x: width, y: height (render-target px)
};

// Push-constant block shared by all Velora shaders (128 bytes, the guaranteed
// minimum maxPushConstantsSize so it runs on every Vulkan device).
struct PushBlock {
  glm::mat4 model;
  glm::vec4 colorA;       // baseColor.rgb, bands
  glm::vec4 colorB;       // specColor.rgb, shininess
  glm::vec4 params;       // rimStrength, rimPower, outlineWidth, unused
  glm::vec4 outlineColor; // outlineColor.rgb, unused
};

// Offscreen NPR renderer: renders a Scene with a chosen style to a PNG.
// The scene is drawn twice — once with the inverted-hull outline pipeline,
// once with the selected style pipeline — into a supersampled target that is
// box-filtered down to the requested resolution on readback.
class Renderer {
public:
  Renderer(VulkanContext& ctx, uint32_t width, uint32_t height, int ssaa,
           std::string shaderDir);
  ~Renderer();

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  void render(const Scene& scene, const std::string& style, float azimuthOffsetDeg,
              const std::string& outPath);

private:
  void createRenderPass();
  void createTargets();
  void createDescriptors();
  void createPipelines();
  void drawNode(const Node& node);
  PushBlock makePush(const Node& node) const;

  VulkanContext& ctx_;
  uint32_t outW_, outH_;   // final image size
  uint32_t rw_, rh_;       // internal render size (supersampled)
  int ssaa_;
  std::string shaderDir_;

  VkRenderPass renderPass_ = VK_NULL_HANDLE;
  Image color_, depth_;
  VkFramebuffer framebuffer_ = VK_NULL_HANDLE;
  Buffer staging_;         // readback
  Buffer ubo_;
  void* stagingMapped_ = nullptr;
  void* uboMapped_ = nullptr;

  VkDescriptorSetLayout setLayout_ = VK_NULL_HANDLE;
  VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
  VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;

  std::unordered_map<std::string, VkPipeline> stylePipelines_;
  VkPipeline outlinePipeline_ = VK_NULL_HANDLE;

  VkCommandBuffer cmd_ = VK_NULL_HANDLE;
  MeshStore meshes_;
};

} // namespace velora
