#pragma once

#include "HalcyonExport.hpp"
#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.h>
#include "GraphicsCore/VulkanDevice.hpp"
#include "GraphicsCore/SamplerDesc.hpp"
#include "GraphicsCore/ImageDesc.hpp"
#include <vector>

struct HALCYON_API StagingBuffer
{
	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;
};

struct HALCYON_API AllocatedImage
{
	vk::Image image;
	VmaAllocation allocation = {};
};

// Stateless Vulkan helpers — buffer/image creation, memory queries, one-shot command buffers.
class HALCYON_API VulkanUtils
{
public:
	static std::string nameFromPath(const std::string& path);
	static std::string resolveShaderDir();
	static std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> createBuffer(vk::DeviceSize size,
	                                                                        vk::BufferUsageFlags usage,
	                                                                        vk::MemoryPropertyFlags properties,
	                                                                        VulkanDevice& vulkanDevice);
	static uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties, VulkanDevice& vulkanDevice);
	static void copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size,
	                       VulkanDevice& vulkanDevice);
	static std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>
	mergeBuffers(vk::raii::Buffer& firstBuffer, vk::DeviceSize firstSize, vk::raii::Buffer& secondBuffer,
	             vk::DeviceSize secondSize, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
	             VulkanDevice& vulkanDevice);
	static std::vector<char> readFile(const std::string& filename);

	static AllocatedImage createImage(VmaAllocator allocator, const ImageDesc& desc);

	static void endSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer, VulkanDevice& vulkanDevice);
	static vk::raii::CommandBuffer beginSingleTimeCommands(VulkanDevice& vulkanDevice);
	static vk::raii::ImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags,
	                                           VulkanDevice& vulkanDevice,
	                                           vk::ImageViewType viewType = vk::ImageViewType::e2D,
	                                           uint32_t mipLevels = 1, uint32_t baseMipLevel = 0,
	                                           uint32_t layerCount = 1, uint32_t baseArrayLayer = 0);
	static void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height,
	                              VulkanDevice& vulkanDevice, uint32_t layerCount = 1);
	static vk::Sampler createSampler(VulkanDevice& vulkanDevice, const SamplerDesc& desc);
	static void transitionImageLayout(vk::raii::CommandBuffer& commandBuffer, vk::Image image, vk::ImageLayout oldLayout,
	                                  vk::ImageLayout newLayout, vk::AccessFlags2 srcAccessMask,
	                                  vk::AccessFlags2 dstAccessMask, vk::PipelineStageFlags2 srcStageMask,
	                                  vk::PipelineStageFlags2 dstStageMask, vk::ImageAspectFlags imageAspectFlags,
	                                  uint32_t layerCount, uint32_t mipLevelCount);
	static StagingBuffer createStagingBuffer(const void* data, vk::DeviceSize size, VmaAllocator allocator,
	                                         VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	static void destroyStagingBuffer(StagingBuffer& stagingBuffer, VmaAllocator allocator);
};