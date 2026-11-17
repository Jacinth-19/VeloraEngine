#pragma once

#include "vk/VulkanContext.hpp"

namespace velora {

struct Buffer {
  VkBuffer buffer = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkDeviceSize size = 0;
};

Buffer createBuffer(VulkanContext& ctx, VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags memFlags);
void destroyBuffer(VulkanContext& ctx, Buffer& buf);
void* mapBuffer(VulkanContext& ctx, Buffer& buf);
void unmapBuffer(VulkanContext& ctx, Buffer& buf);
void copyBuffer(VulkanContext& ctx, Buffer& src, Buffer& dst, VkDeviceSize size);

struct Image {
  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
  VkFormat format = VK_FORMAT_UNDEFINED;
  uint32_t width = 0;
  uint32_t height = 0;
};

Image createImage2D(VulkanContext& ctx, uint32_t width, uint32_t height, VkFormat format,
                    VkImageUsageFlags usage, VkImageAspectFlags aspect);
void destroyImage(VulkanContext& ctx, Image& img);

} // namespace velora
