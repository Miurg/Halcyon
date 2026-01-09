#include "VulkanUtils.hpp"
#include <iostream>
#include <fstream>

/// <summary>
/// Creates a Vulkan buffer with the specified size, usage flags, and memory properties, and allocates device memory for
/// it.
/// </summary>
/// <param name="size">The size in bytes of the buffer to create (vk::DeviceSize).</param>
/// <param name="usage">Buffer usage flags specifying how the buffer will be used (e.g., eVertexBuffer, eIndexBuffer,
/// eTransferSrc, eTransferDst).</param> <param name="properties">Memory property flags specifying the required memory
/// properties (e.g., eDeviceLocal, eHostVisible, eHostCoherent).</param> <param name="vulkanDevice">Reference to the
/// VulkanDevice wrapper used to create the buffer and allocate memory.</param> <returns>A pair containing the created
/// buffer and its associated device memory.</returns>
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

/// <summary>
/// Finds a physical device memory type index that satisfies the provided type filter and memory property flags.
/// </summary>
/// <param name="typeFilter">A bitmask where each bit represents a memory type that is compatible (typically from
/// VkMemoryRequirements::memoryTypeBits).</param> <param name="properties">vk::MemoryPropertyFlags specifying required
/// memory properties (for example, device local or host visible).</param> <param name="vulkanDevice">Reference to a
/// VulkanDevice instance used to query the physical device's memory properties.</param> <returns>The index (uint32_t)
/// of a suitable memory type that matches the typeFilter and includes the requested properties.</returns>
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

/// <summary>
/// Copies data from a source Vulkan buffer to a destination Vulkan buffer using a single-use command buffer.
/// </summary>
/// <param name="srcBuffer">Reference to the source vk::raii::Buffer to copy data from.</param>
/// <param name="dstBuffer">Reference to the destination vk::raii::Buffer to copy data to.</param>
/// <param name="size">Number of bytes to copy (vk::DeviceSize).</param>
/// <param name="vulkanDevice">Reference to the VulkanDevice wrapper used to begin and end the single-time command
/// buffer and submit the copy.</param>
void VulkanUtils::copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size,
                             VulkanDevice& vulkanDevice)
{
	vk::raii::CommandBuffer commandCopyBuffer = beginSingleTimeCommands(vulkanDevice);
	commandCopyBuffer.copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, 0, size));
	endSingleTimeCommands(commandCopyBuffer, vulkanDevice);
}

/// <summary>
/// Merges two Vulkan buffers into a single new buffer by copying data from both source buffers sequentially.
/// </summary>
/// <param name="firstBuffer">Reference to the first source buffer to copy data from.</param>
/// <param name="firstSize">Size in bytes of the first buffer.</param>
/// <param name="secondBuffer">Reference to the second source buffer to copy data from.</param>
/// <param name="secondSize">Size in bytes of the second buffer.</param>
/// <param name="usage">Buffer usage flags for the merged buffer (e.g., eVertexBuffer, eIndexBuffer,
/// eTransferDst).</param> <param name="properties">Memory property flags for the merged buffer (e.g.,
/// eDeviceLocal).</param> <param name="vulkanDevice">Reference to the VulkanDevice wrapper used for buffer creation and
/// copying.</param> <returns>A pair containing the merged buffer and its associated device memory.</returns>
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

/// <summary>
/// Allocates and begins a primary command buffer for single-use commands from the provided Vulkan device.
/// </summary>
/// <param name="vulkanDevice">Reference to a VulkanDevice that provides the Vulkan device and command pool used to
/// allocate the command buffer.</param> <returns>A vk::raii::CommandBuffer that has been allocated from the device's
/// command pool and begun with the OneTimeSubmit usage flag, ready for recording single-use commands.</returns>
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

/// <summary>
/// Ends recording of a single-use command buffer, submits it to the graphics queue, and waits for completion.
/// </summary>
/// <param name="commandBuffer">Reference to the command buffer that has been recorded and should be ended and
/// submitted.</param> <param name="vulkanDevice">Reference to the VulkanDevice wrapper that provides the graphics queue
/// for submission.</param>
void VulkanUtils::endSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer, VulkanDevice& vulkanDevice)
{
	commandBuffer.end();

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &*commandBuffer;
	vulkanDevice.graphicsQueue.submit(submitInfo, nullptr);
	vulkanDevice.graphicsQueue.waitIdle();
}

/// <summary>
/// Reads the entire contents of a binary file into a vector of characters.
/// </summary>
/// <param name="filename">The path to the file to read.</param>
/// <returns>A vector containing the file's binary data as characters.</returns>
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

/// <summary>
/// Creates a 2D Vulkan image with the specified dimensions, format, tiling, usage flags, and memory properties, and
/// allocates device memory for it.
/// </summary>
/// <param name="width">The width of the image in pixels.</param>
/// <param name="height">The height of the image in pixels.</param>
/// <param name="format">The pixel format of the image (e.g., eR8G8B8A8Srgb, eD32Sfloat).</param>
/// <param name="tiling">The image tiling mode (e.g., eOptimal, eLinear).</param>
/// <param name="usage">Image usage flags specifying how the image will be used (e.g., eSampled, eTransferDst,
/// eColorAttachment).</param> <param name="properties">Memory property flags specifying the required memory properties
/// (e.g., eDeviceLocal).</param> <param name="image">Reference to the vk::raii::Image object that will be
/// created.</param> <param name="imageMemory">Reference to the vk::raii::DeviceMemory object that will be allocated for
/// the image.</param> <param name="vulkanDevice">Reference to the VulkanDevice wrapper used to create the image and
/// allocate memory.</param>
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

/// <summary>
/// Creates a 2D image view for a Vulkan image with the specified format and aspect flags.
/// </summary>
/// <param name="image">The Vulkan image to create a view for.</param>
/// <param name="format">The pixel format of the image view (should match the image format).</param>
/// <param name="aspectFlags">Image aspect flags specifying which aspects of the image are included in the view (e.g.,
/// eColor, eDepth).</param> <param name="vulkanDevice">Reference to the VulkanDevice wrapper used to create the image
/// view.</param> <returns>A vk::raii::ImageView object representing the created image view.</returns>
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