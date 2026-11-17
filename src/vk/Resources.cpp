#include "vk/Resources.hpp"

namespace velora {

Buffer createBuffer(VulkanContext& ctx, VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags memFlags) {
  Buffer buf;
  buf.size = size;

  VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bci.size = size;
  bci.usage = usage;
  bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  VK_CHECK(vkCreateBuffer(ctx.device, &bci, nullptr, &buf.buffer));

  VkMemoryRequirements req{};
  vkGetBufferMemoryRequirements(ctx.device, buf.buffer, &req);

  VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  mai.allocationSize = req.size;
  mai.memoryTypeIndex = ctx.findMemoryType(req.memoryTypeBits, memFlags);
  VK_CHECK(vkAllocateMemory(ctx.device, &mai, nullptr, &buf.memory));
  VK_CHECK(vkBindBufferMemory(ctx.device, buf.buffer, buf.memory, 0));
  return buf;
}

void destroyBuffer(VulkanContext& ctx, Buffer& buf) {
  if (buf.buffer != VK_NULL_HANDLE) vkDestroyBuffer(ctx.device, buf.buffer, nullptr);
  if (buf.memory != VK_NULL_HANDLE) vkFreeMemory(ctx.device, buf.memory, nullptr);
  buf = Buffer{};
}

void* mapBuffer(VulkanContext& ctx, Buffer& buf) {
  void* ptr = nullptr;
  VK_CHECK(vkMapMemory(ctx.device, buf.memory, 0, buf.size, 0, &ptr));
  return ptr;
}

void unmapBuffer(VulkanContext& ctx, Buffer& buf) {
  vkUnmapMemory(ctx.device, buf.memory);
}

void copyBuffer(VulkanContext& ctx, Buffer& src, Buffer& dst, VkDeviceSize size) {
  VkCommandBuffer cmd = ctx.beginOneShot();
  VkBufferCopy region{0, 0, size};
  vkCmdCopyBuffer(cmd, src.buffer, dst.buffer, 1, &region);
  ctx.endOneShot(cmd);
}

Image createImage2D(VulkanContext& ctx, uint32_t width, uint32_t height, VkFormat format,
                    VkImageUsageFlags usage, VkImageAspectFlags aspect) {
  Image img;
  img.width = width;
  img.height = height;
  img.format = format;

  VkImageCreateInfo ici{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  ici.imageType = VK_IMAGE_TYPE_2D;
  ici.format = format;
  ici.extent = {width, height, 1};
  ici.mipLevels = 1;
  ici.arrayLayers = 1;
  ici.samples = VK_SAMPLE_COUNT_1_BIT;
  ici.tiling = VK_IMAGE_TILING_OPTIMAL;
  ici.usage = usage;
  ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  VK_CHECK(vkCreateImage(ctx.device, &ici, nullptr, &img.image));

  VkMemoryRequirements req{};
  vkGetImageMemoryRequirements(ctx.device, img.image, &req);

  VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  mai.allocationSize = req.size;
  mai.memoryTypeIndex = ctx.findMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VK_CHECK(vkAllocateMemory(ctx.device, &mai, nullptr, &img.memory));
  VK_CHECK(vkBindImageMemory(ctx.device, img.image, img.memory, 0));

  VkImageViewCreateInfo vci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  vci.image = img.image;
  vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
  vci.format = format;
  vci.subresourceRange = {aspect, 0, 1, 0, 1};
  VK_CHECK(vkCreateImageView(ctx.device, &vci, nullptr, &img.view));
  return img;
}

void destroyImage(VulkanContext& ctx, Image& img) {
  if (img.view != VK_NULL_HANDLE) vkDestroyImageView(ctx.device, img.view, nullptr);
  if (img.image != VK_NULL_HANDLE) vkDestroyImage(ctx.device, img.image, nullptr);
  if (img.memory != VK_NULL_HANDLE) vkFreeMemory(ctx.device, img.memory, nullptr);
  img = Image{};
}

} // namespace velora
