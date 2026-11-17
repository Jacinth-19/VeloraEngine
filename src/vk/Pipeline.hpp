#pragma once

#include "vk/VulkanContext.hpp"

#include <string>

namespace velora {

struct GraphicsPipelineDesc {
  std::string vertShader; // path to GLSL vertex shader
  std::string fragShader; // path to GLSL fragment shader
  VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
  VkBool32 depthWrite = VK_TRUE;
  VkCompareOp depthCompare = VK_COMPARE_OP_LESS;
};

// Builds a graphics pipeline for Velora's fixed vertex format
// (vec3 position + vec3 normal), with dynamic viewport/scissor.
VkPipeline createGraphicsPipeline(VulkanContext& ctx, VkRenderPass renderPass,
                                  VkPipelineLayout layout,
                                  const GraphicsPipelineDesc& desc);

} // namespace velora
