#include "vk/VulkanContext.hpp"

#include "core/Log.hpp"

#include <cstdio>
#include <cstdlib>
#include <vector>

namespace velora {

static const char* resultName(VkResult r) {
  switch (r) {
    case VK_SUCCESS: return "VK_SUCCESS";
    case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    default: return "VK_ERROR_<unknown>";
  }
}

void vkFatal(const char* expr, VkResult res, const char* file, int line) {
  std::fprintf(stderr, "\033[31m[ERROR]\033[0m Vulkan call failed: %s -> %s (%d) at %s:%d\n",
               expr, resultName(res), (int)res, file, line);
  std::exit(1);
}

void VulkanContext::init() {
  VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
  app.pApplicationName = "Velora Engine";
  app.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  app.pEngineName = "Velora";
  app.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  app.apiVersion = VK_API_VERSION_1_2;

  VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  ici.pApplicationInfo = &app;
  // No instance extensions: rendering is fully offscreen/headless.
  VK_CHECK(vkCreateInstance(&ici, nullptr, &instance));

  uint32_t count = 0;
  vkEnumeratePhysicalDevices(instance, &count, nullptr);
  if (count == 0)
    log::fatal("no Vulkan physical devices found. Point VK_ICD_FILENAMES at a valid "
               "ICD (tools/fetch_deps.sh sets up SwiftShader for headless machines).");

  std::vector<VkPhysicalDevice> devices(count);
  vkEnumeratePhysicalDevices(instance, &count, devices.data());

  log::info("found %u Vulkan device(s):", count);
  for (uint32_t i = 0; i < count; i++) {
    VkPhysicalDeviceProperties p{};
    vkGetPhysicalDeviceProperties(devices[i], &p);
    log::info("  [%u] %s (api %u.%u.%u, type %d)", i, p.deviceName,
              VK_VERSION_MAJOR(p.apiVersion), VK_VERSION_MINOR(p.apiVersion),
              VK_VERSION_PATCH(p.apiVersion), (int)p.deviceType);
  }

  // Prefer a discrete GPU if present, otherwise take the first device
  // (SwiftShader/lavapipe show up as CPU devices).
  physical = devices[0];
  for (auto d : devices) {
    VkPhysicalDeviceProperties p{};
    vkGetPhysicalDeviceProperties(d, &p);
    if (p.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) { physical = d; break; }
  }

  vkGetPhysicalDeviceProperties(physical, &props);
  vkGetPhysicalDeviceMemoryProperties(physical, &memProps);

  uint32_t qfCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical, &qfCount, nullptr);
  std::vector<VkQueueFamilyProperties> qf(qfCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physical, &qfCount, qf.data());

  queueFamily = UINT32_MAX;
  for (uint32_t i = 0; i < qfCount; i++) {
    if (qf[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) { queueFamily = i; break; }
  }
  if (queueFamily == UINT32_MAX)
    log::fatal("device exposes no graphics queue family");

  float prio = 1.0f;
  VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  qci.queueFamilyIndex = queueFamily;
  qci.queueCount = 1;
  qci.pQueuePriorities = &prio;

  VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  dci.queueCreateInfoCount = 1;
  dci.pQueueCreateInfos = &qci;
  VK_CHECK(vkCreateDevice(physical, &dci, nullptr, &device));

  vkGetDeviceQueue(device, queueFamily, 0, &queue);

  VkCommandPoolCreateInfo cpi{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  cpi.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  cpi.queueFamilyIndex = queueFamily;
  VK_CHECK(vkCreateCommandPool(device, &cpi, nullptr, &cmdPool));

  log::info("using device: %s", props.deviceName);
}

void VulkanContext::shutdown() {
  if (device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(device);
    vkDestroyCommandPool(device, cmdPool, nullptr);
    vkDestroyDevice(device, nullptr);
    device = VK_NULL_HANDLE;
  }
  if (instance != VK_NULL_HANDLE) {
    vkDestroyInstance(instance, nullptr);
    instance = VK_NULL_HANDLE;
  }
}

VkCommandBuffer VulkanContext::beginOneShot() {
  VkCommandBufferAllocateInfo cai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  cai.commandPool = cmdPool;
  cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cai.commandBufferCount = 1;
  VkCommandBuffer cmd;
  VK_CHECK(vkAllocateCommandBuffers(device, &cai, &cmd));

  VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  VK_CHECK(vkBeginCommandBuffer(cmd, &bi));
  return cmd;
}

void VulkanContext::endOneShot(VkCommandBuffer cmd) {
  VK_CHECK(vkEndCommandBuffer(cmd));
  VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  si.commandBufferCount = 1;
  si.pCommandBuffers = &cmd;
  VK_CHECK(vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE));
  VK_CHECK(vkQueueWaitIdle(queue));
  vkFreeCommandBuffers(device, cmdPool, 1, &cmd);
}

uint32_t VulkanContext::findMemoryType(uint32_t typeFilter,
                                       VkMemoryPropertyFlags props) const {
  for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
    if ((typeFilter & (1u << i)) &&
        (memProps.memoryTypes[i].propertyFlags & props) == props)
      return i;
  }
  log::fatal("no suitable Vulkan memory type");
}

} // namespace velora
