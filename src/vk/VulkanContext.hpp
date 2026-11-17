#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace velora {

// Owns the Vulkan instance, physical/logical device, a single graphics queue
// and a command pool. Velora renders offscreen, so no surface/swapchain is
// required — this works headless (CI, servers) and with any ICD, including
// software rasterizers such as SwiftShader/lavapipe.
class VulkanContext {
public:
  void init();
  void shutdown();

  VkCommandBuffer beginOneShot();
  void endOneShot(VkCommandBuffer cmd);

  uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props) const;

  VkInstance instance = VK_NULL_HANDLE;
  VkPhysicalDevice physical = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkQueue queue = VK_NULL_HANDLE;
  uint32_t queueFamily = 0;
  VkCommandPool cmdPool = VK_NULL_HANDLE;

  VkPhysicalDeviceProperties props{};
  VkPhysicalDeviceMemoryProperties memProps{};
};

[[noreturn]] void vkFatal(const char* expr, VkResult res, const char* file, int line);

#define VK_CHECK(expr)                                              \
  do {                                                              \
    VkResult _r = (expr);                                           \
    if (_r != VK_SUCCESS)                                           \
      ::velora::vkFatal(#expr, _r, __FILE__, __LINE__);             \
  } while (0)

} // namespace velora
