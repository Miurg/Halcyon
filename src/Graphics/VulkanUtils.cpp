#include "VulkanUtils.hpp"
#include <fstream>

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
void VulkanUtils::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
                              vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Image& image,
                              vk::raii::DeviceMemory& imageMemory, VulkanDevice& vulkanDevice, uint32_t mipLevels,
                              uint32_t arrayLayers, vk::ImageCreateFlags flags)
{
	vk::ImageCreateInfo imageInfo;
	imageInfo.flags = flags;
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.format = format;
	imageInfo.extent = vk::Extent3D{width, height, 1};
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = arrayLayers;
	imageInfo.samples = vk::SampleCountFlagBits::e1;
	imageInfo.tiling = tiling;
	imageInfo.usage = usage;
	imageInfo.sharingMode = vk::SharingMode::eExclusive;

	image = vk::raii::Image(vulkanDevice.device, imageInfo);

	vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
	vk::MemoryAllocateInfo allocInfo(
	    memRequirements.size, VulkanUtils::findMemoryType(memRequirements.memoryTypeBits, properties, vulkanDevice));
	imageMemory = vk::raii::DeviceMemory(vulkanDevice.device, allocInfo);
	image.bindMemory(*imageMemory, 0);
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