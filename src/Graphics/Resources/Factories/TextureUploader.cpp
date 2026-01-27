#include "TextureUploader.hpp"
#include "../../VulkanUtils.hpp"
#include <stb_image.h>


void TextureUploader::uploadTextureFromFile(const char* texturePath, Texture& texture, VmaAllocator& allocator,
                                            VulkanDevice& vulkanDevice)
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(texturePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels)
	{
		throw std::runtime_error("failed to load texture image!");
	}

	uploadTextureFromBuffer(pixels, texWidth, texHeight, texture, allocator, vulkanDevice);

	stbi_image_free(pixels);
}

void TextureUploader::transitionImageLayout(vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                                            VulkanDevice& vulkanDevice)
{
	auto commandBuffer = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

	vk::PipelineStageFlags sourceStage;
	vk::PipelineStageFlags destinationStage;

	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
	{
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else
	{
		throw std::invalid_argument("unsupported layout transition!");
	}
	commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, {}, barrier);

	VulkanUtils::endSingleTimeCommands(commandBuffer, vulkanDevice);
}

void TextureUploader::uploadTextureFromBuffer(const unsigned char* pixels, int texWidth, int texHeight,
                                              Texture& texture, VmaAllocator& allocator, VulkanDevice& vulkanDevice)
{
	if (!pixels)
	{
		throw std::runtime_error("pixels data is null!");
	}

	if (texWidth <= 0 || texHeight <= 0)
	{
		throw std::runtime_error("Invalid texture dimensions!");
	}

	// Create staging buffer (RGBA)
	vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(texWidth) * static_cast<vk::DeviceSize>(texHeight) * 4;

	VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	bufferInfo.size = imageSize;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	VmaAllocationInfo stagingAllocInfo;

	VkResult result =
	    vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &stagingBuffer, &stagingAllocation, &stagingAllocInfo);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create staging buffer for texture upload! VkResult: " +
		                         std::to_string(result));
	}

	if (!stagingAllocInfo.pMappedData)
	{
		vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
		throw std::runtime_error("Failed to map staging buffer memory!");
	}

	memcpy(stagingAllocInfo.pMappedData, pixels, static_cast<size_t>(imageSize));

	transitionImageLayout(texture.textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
	                      vulkanDevice);

	VulkanUtils::copyBufferToImage(stagingBuffer, texture.textureImage, static_cast<uint32_t>(texWidth),
	                               static_cast<uint32_t>(texHeight), vulkanDevice);

	transitionImageLayout(texture.textureImage, vk::ImageLayout::eTransferDstOptimal,
	                      vk::ImageLayout::eShaderReadOnlyOptimal, vulkanDevice);

	vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
}

