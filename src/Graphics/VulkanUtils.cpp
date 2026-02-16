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

// Creates a 2D image and allocates memory for it.
void VulkanUtils::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
                              vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Image& image,
                              vk::raii::DeviceMemory& imageMemory, VulkanDevice& vulkanDevice)
{
	vk::ImageCreateInfo imageInfo;
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.format = format;
	imageInfo.extent = vk::Extent3D{width, height, 1};
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
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

// Creates a 2D image view.
vk::raii::ImageView VulkanUtils::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags,
                                                 VulkanDevice& vulkanDevice)
{
	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = image;
	viewInfo.viewType = vk::ImageViewType::e2D;
	viewInfo.format = format;
	viewInfo.subresourceRange = {aspectFlags, 0, 1, 0, 1};
	return vk::raii::ImageView(vulkanDevice.device, viewInfo);
}

void VulkanUtils::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height,
                                    VulkanDevice& vulkanDevice)
{
	vk::raii::CommandBuffer commandBuffer = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

	vk::BufferImageCopy region;
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1};
	region.imageOffset = vk::Offset3D{0, 0, 0};
	region.imageExtent = vk::Extent3D{width, height, 1};

	commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, {region});

	VulkanUtils::endSingleTimeCommands(commandBuffer, vulkanDevice);
}