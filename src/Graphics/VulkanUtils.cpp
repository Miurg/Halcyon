#include "VulkanUtils.hpp"
#include <iostream>
#include <fstream>

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

void VulkanUtils::copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size,
                             VulkanDevice& vulkanDevice)
{
	vk::raii::CommandBuffer commandCopyBuffer = beginSingleTimeCommands(vulkanDevice);
	commandCopyBuffer.copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, 0, size));
	endSingleTimeCommands(commandCopyBuffer, vulkanDevice);
}

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

void VulkanUtils::endSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer, VulkanDevice& vulkanDevice)
{
	commandBuffer.end();

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &*commandBuffer;
	vulkanDevice.graphicsQueue.submit(submitInfo, nullptr);
	vulkanDevice.graphicsQueue.waitIdle();
}

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