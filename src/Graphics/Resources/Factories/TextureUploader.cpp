#include "TextureUploader.hpp"
#include "TextureUploader.hpp"
#include "../../VulkanUtils.hpp"
#include "../Managers/TextureManager.hpp"
#include <stb_image.h>
#include <iostream>

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
	barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, vk::RemainingMipLevels, 0, 1};

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

	generateMipmaps(texture.textureImage, texture.format, texWidth, texHeight, texture.mipLevels, vulkanDevice);

	vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
}

void TextureUploader::uploadHdrTextureFromBuffer(const float* pixels, int texWidth, int texHeight, Texture& texture,
                                                 VmaAllocator& allocator, VulkanDevice& vulkanDevice)
{
	if (!pixels)
	{
		throw std::runtime_error("pixels data is null!");
	}

	if (texWidth <= 0 || texHeight <= 0)
	{
		throw std::runtime_error("Invalid texture dimensions!");
	}

	// Create staging buffer (RGBA32F, 16 bytes per pixel)
	vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(texWidth) * static_cast<vk::DeviceSize>(texHeight) * 16;

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
		throw std::runtime_error("Failed to create staging buffer for HDR texture upload!");
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

	generateMipmaps(texture.textureImage, texture.format, texWidth, texHeight, texture.mipLevels, vulkanDevice);

	vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
}

void TextureUploader::uploadHdrTextureFromFile(const char* texturePath, Texture& texture, TextureManager& tManager,
                                               VmaAllocator& allocator, VulkanDevice& vulkanDevice)
{
	int hdrWidth, hdrHeight, hdrChannels;
	stbi_set_flip_vertically_on_load(true); 
	float* hdrPixels = stbi_loadf(texturePath, &hdrWidth, &hdrHeight, &hdrChannels, STBI_rgb_alpha);
	stbi_set_flip_vertically_on_load(false); // Duplication is intentional

	float fallbackPixel[4] = {0.5f, 0.5f, 0.5f, 1.0f};
	bool freePixels = true;
	if (!hdrPixels)
	{
		std::cout << "Warning: Could not load HDR skybox from " << texturePath << ". Using fallback 1x1 pixel."
		          << std::endl;
		hdrPixels = fallbackPixel;
		hdrWidth = 1;
		hdrHeight = 1;
		freePixels = false;
	}

	uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(hdrWidth, hdrHeight)))) + 1;
	tManager.createImage(hdrWidth, hdrHeight, vk::Format::eR32G32B32A32Sfloat, vk::ImageTiling::eOptimal,
	                     vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst |
	                         vk::ImageUsageFlagBits::eSampled,
	                     VMA_MEMORY_USAGE_AUTO, texture, mipLevels);

	uploadHdrTextureFromBuffer(hdrPixels, hdrWidth, hdrHeight, texture, allocator, vulkanDevice);

	tManager.createImageView(texture, vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor);
	tManager.createTextureSampler(texture);

	if (freePixels)
	{
		stbi_image_free(hdrPixels);
	}
}

void TextureUploader::generateMipmaps(vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight,
                                      uint32_t mipLevels, VulkanDevice& vulkanDevice)
{
	vk::FormatProperties formatProperties = vulkanDevice.physicalDevice.getFormatProperties(imageFormat);
	if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
	{
		throw std::runtime_error("texture image format does not support linear blitting!");
	}

	auto commandBuffer = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

	vk::ImageMemoryBarrier barrier;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	for (uint32_t i = 1; i < mipLevels; i++)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {},
		                              {}, barrier);

		vk::ImageBlit blit;
		blit.srcOffsets[0] = vk::Offset3D{0, 0, 0};
		blit.srcOffsets[1] = vk::Offset3D{mipWidth, mipHeight, 1};
		blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;

		blit.dstOffsets[0] = vk::Offset3D{0, 0, 0};
		blit.dstOffsets[1] = vk::Offset3D{mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
		blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal,
		                        blit, vk::Filter::eLinear);

		barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
		barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
		                              {}, {}, {}, barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
	barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
	barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

	commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {},
	                              {}, {}, barrier);

	VulkanUtils::endSingleTimeCommands(commandBuffer, vulkanDevice);
}
