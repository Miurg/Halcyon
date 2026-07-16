#include "GraphicsCore/VulkanUtils.hpp"
#include "GraphicsCore/VulkanConvert.hpp"
#include "PlatformCore/Platform.hpp"
#include <fstream>
#include <filesystem>
#include <algorithm>

vk::Sampler VulkanUtils::createSampler(VulkanDevice& vulkanDevice, const SamplerDesc& desc, uint32_t mipLevels)
{
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = VulkanConvert::toVkFilter(desc.magFilter);
	samplerInfo.minFilter = VulkanConvert::toVkFilter(desc.minFilter);
	samplerInfo.mipmapMode = VulkanConvert::toVkMipmapMode(desc.mipmapMode);

	vk::SamplerAddressMode addressMode = VulkanConvert::toVkAddressMode(desc.addressMode);
	samplerInfo.addressModeU = addressMode;
	samplerInfo.addressModeV = addressMode;
	samplerInfo.addressModeW = addressMode;
	samplerInfo.borderColor = VulkanConvert::toVkBorderColor(desc.borderColor);

	if (desc.maxAnisotropy > 1.0f)
	{
		samplerInfo.anisotropyEnable = vk::True;
		samplerInfo.maxAnisotropy =
		    std::min(desc.maxAnisotropy, vulkanDevice.physicalDevice.getProperties().limits.maxSamplerAnisotropy);
	}
	else
	{
		samplerInfo.anisotropyEnable = vk::False;
		samplerInfo.maxAnisotropy = 1.0f;
	}

	samplerInfo.compareEnable = desc.compareOp == SamplerCompareOp::None ? vk::False : vk::True;
	samplerInfo.compareOp = VulkanConvert::toVkCompareOp(desc.compareOp);

	samplerInfo.mipLodBias = desc.mipLodBias;
	samplerInfo.minLod = desc.minLod;
	samplerInfo.maxLod = desc.maxLod == SamplerMaxLod::FullChain ? VK_LOD_CLAMP_NONE
	                                                             : static_cast<float>(std::max(1u, mipLevels));

	return (*vulkanDevice.device).createSampler(samplerInfo);
}

std::string VulkanUtils::nameFromPath(const std::string& path)
{
	return std::filesystem::path(path).stem().string(); // "shaders/mesh.slang" -> "mesh"
}

std::string VulkanUtils::resolveShaderDir()
{
	if (std::string env = Platform::getEnv("HALCYON_SHADER_DIR"); !env.empty()) return env;

	if (std::string exeDir = Platform::executableDir(); !exeDir.empty())
	{
		std::filesystem::path candidate = std::filesystem::path(exeDir) / "shaders";
		if (std::filesystem::exists(candidate)) return candidate.string();
	}

	return HALCYON_SHADER_OUT_DIR;
}

// Creates a buffer and allocates memory for it.
std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> VulkanUtils::createBuffer(vk::DeviceSize size,
                                                                              vk::BufferUsageFlags usage,
                                                                              vk::MemoryPropertyFlags properties,
                                                                              VulkanDevice& vulkanDevice)
{
	vk::BufferCreateInfo bufferInfo;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;

	vk::raii::Buffer buffer(vulkanDevice.device, bufferInfo);

	vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
	vk::MemoryAllocateInfo memoryAllocateInfo;
	memoryAllocateInfo.allocationSize = memRequirements.size;
	memoryAllocateInfo.memoryTypeIndex =
	    VulkanUtils::findMemoryType(memRequirements.memoryTypeBits, properties, vulkanDevice);

	vk::raii::DeviceMemory bufferMemory(vulkanDevice.device, memoryAllocateInfo);

	buffer.bindMemory(*bufferMemory, 0);
	return {std::move(buffer), std::move(bufferMemory)};
}

// Finds a memory type index matching the filter and required properties.
uint32_t VulkanUtils::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties,
                                     VulkanDevice& vulkanDevice)
{
	vk::PhysicalDeviceMemoryProperties memProperties = vulkanDevice.physicalDevice.getMemoryProperties();

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

// Copies data between buffers via a one-shot command buffer.
void VulkanUtils::copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size,
                             VulkanDevice& vulkanDevice)
{
	vk::raii::CommandBuffer commandCopyBuffer = beginSingleTimeCommands(vulkanDevice);
	commandCopyBuffer.copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, 0, size));
	endSingleTimeCommands(commandCopyBuffer, vulkanDevice);
}

// Merges two buffers into one by copying their data sequentially.
std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>
VulkanUtils::mergeBuffers(vk::raii::Buffer& firstBuffer, vk::DeviceSize firstSize, vk::raii::Buffer& secondBuffer,
                          vk::DeviceSize secondSize, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
                          VulkanDevice& vulkanDevice)
{
	vk::DeviceSize mergedSize = firstSize + secondSize;
	auto [mergedBuffer, mergedBufferMemory] =
	    createBuffer(mergedSize, usage | vk::BufferUsageFlagBits::eTransferDst, properties, vulkanDevice);

	vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands(vulkanDevice);

	commandBuffer.copyBuffer(firstBuffer, mergedBuffer, vk::BufferCopy(0, 0, firstSize));
	commandBuffer.copyBuffer(secondBuffer, mergedBuffer, vk::BufferCopy(0, firstSize, secondSize));

	endSingleTimeCommands(commandBuffer, vulkanDevice);
	return {std::move(mergedBuffer), std::move(mergedBufferMemory)};
}

// Allocates and begins a one-shot command buffer.
vk::raii::CommandBuffer VulkanUtils::beginSingleTimeCommands(VulkanDevice& vulkanDevice)
{
	vk::CommandBufferAllocateInfo allocInfo;
	allocInfo.commandPool = vulkanDevice.commandPool;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandBufferCount = 1;
	vk::raii::CommandBuffer commandBuffer = std::move(vulkanDevice.device.allocateCommandBuffers(allocInfo).front());

	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	commandBuffer.begin(beginInfo);

	return commandBuffer;
}

// Ends, submits and waits for a one-shot command buffer.
void VulkanUtils::endSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer, VulkanDevice& vulkanDevice)
{
	commandBuffer.end();

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &*commandBuffer;
	vulkanDevice.graphicsQueue.submit(submitInfo, nullptr);
	vulkanDevice.graphicsQueue.waitIdle();
}

// Reads a binary file into a char vector.
std::vector<char> VulkanUtils::readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file!");
	}

	std::vector<char> buffer(file.tellg());
	file.seekg(0, std::ios::beg);
	file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
	file.close();

	return buffer;
}

// Creates a 2D or Cubemap image and allocates memory for it.
AllocatedImage VulkanUtils::createImage(VmaAllocator allocator, const ImageDesc& desc)
{
	if (desc.width == 0 || desc.height == 0)
	{
		throw std::runtime_error("failed to create VMA image: invalid image dimensions (width or height is 0)!");
	}

	VkImageCreateInfo imageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	imageInfo.flags = static_cast<VkImageCreateFlags>(desc.flags);
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = desc.width;
	imageInfo.extent.height = desc.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = desc.mipLevels;
	imageInfo.arrayLayers = desc.layerCount;
	imageInfo.format = static_cast<VkFormat>(desc.format);
	imageInfo.tiling = static_cast<VkImageTiling>(desc.tiling);
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = static_cast<VkImageUsageFlags>(desc.usage);
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = static_cast<VkSampleCountFlagBits>(desc.samples);

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = desc.memoryUsage;

	VkImage rawImage;
	AllocatedImage result;
	VkResult res = vmaCreateImage(allocator, &imageInfo, &allocInfo, &rawImage, &result.allocation, nullptr);
	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create VMA image!");
	}
	result.image = vk::Image(rawImage);
	return result;
}

// Creates an image view.
vk::raii::ImageView VulkanUtils::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags,
                                                 VulkanDevice& vulkanDevice, vk::ImageViewType viewType,
                                                 uint32_t mipLevels, uint32_t baseMipLevel, uint32_t layerCount,
                                                 uint32_t baseArrayLayer)
{
	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = image;
	viewInfo.viewType = viewType;
	viewInfo.format = format;
	viewInfo.subresourceRange = {aspectFlags, baseMipLevel, mipLevels, baseArrayLayer, layerCount};
	return vk::raii::ImageView(vulkanDevice.device, viewInfo);
}

void VulkanUtils::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height,
                                    VulkanDevice& vulkanDevice, uint32_t layerCount)
{
	vk::raii::CommandBuffer commandBuffer = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

	vk::BufferImageCopy region;
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, layerCount};
	region.imageOffset = vk::Offset3D{0, 0, 0};
	region.imageExtent = vk::Extent3D{width, height, 1};

	commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, {region});

	VulkanUtils::endSingleTimeCommands(commandBuffer, vulkanDevice);
}

void VulkanUtils::transitionImageLayout(vk::raii::CommandBuffer& commandBuffer, vk::Image image,
                                                 vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                                                 vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask,
                                                 vk::PipelineStageFlags2 srcStageMask,
                                                 vk::PipelineStageFlags2 dstStageMask,
                                                 vk::ImageAspectFlags imageAspectFlags, uint32_t layerCount,
                                                 uint32_t mipLevelCount)
{
	vk::ImageMemoryBarrier2 barrier;
	barrier.srcStageMask = srcStageMask;
	barrier.srcAccessMask = srcAccessMask;
	barrier.dstStageMask = dstStageMask;
	barrier.dstAccessMask = dstAccessMask;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	barrier.subresourceRange.aspectMask = imageAspectFlags;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevelCount;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = layerCount;

	vk::DependencyInfo dependencyInfo;
	dependencyInfo.dependencyFlags = {};
	dependencyInfo.imageMemoryBarrierCount = 1;
	dependencyInfo.pImageMemoryBarriers = &barrier;
	commandBuffer.pipelineBarrier2(dependencyInfo);
}

StagingBuffer VulkanUtils::createStagingBuffer(const void* data, vk::DeviceSize size,
                                                            VmaAllocator allocator, VkBufferUsageFlags usage)
{
	if (!data || size == 0) throw std::runtime_error("Invalid data or size for staging buffer!");

	VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	bufferInfo.size = static_cast<VkDeviceSize>(size);
	bufferInfo.usage = usage;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

	StagingBuffer staging{};
	VmaAllocationInfo stagingAllocInfo;

	VkResult result =
	    vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &staging.buffer, &staging.allocation, &stagingAllocInfo);
	if (result != VK_SUCCESS) throw std::runtime_error("Failed to create staging buffer!");

	if (!stagingAllocInfo.pMappedData)
	{
		destroyStagingBuffer(staging, allocator);
		throw std::runtime_error("Failed to map staging buffer memory!");
	}

	memcpy(stagingAllocInfo.pMappedData, data, static_cast<size_t>(size));
	return staging;
}

void VulkanUtils::destroyStagingBuffer(StagingBuffer& stagingBuffer, VmaAllocator allocator)
{
	if (stagingBuffer.buffer != VK_NULL_HANDLE)
	{
		vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
		stagingBuffer.buffer = VK_NULL_HANDLE;
		stagingBuffer.allocation = VK_NULL_HANDLE;
	}
}