#include "TextureUploader.hpp"
#include "TextureUploader.hpp"
#include "../../VulkanUtils.hpp"
#include "../Managers/TextureManager.hpp"
#include <stb_image.h>
#include <ktx.h>
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

void TextureUploader::uploadKtxTextureData(const unsigned char* ktxData, size_t dataSize, Texture& texture, bool isSrgb,
                                           VmaAllocator& allocator, VulkanDevice& vulkanDevice)
{
	ktxTexture2* ktxTex = nullptr;
	KTX_error_code res =
	    ktxTexture2_CreateFromMemory(ktxData, dataSize, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTex);
	if (res != KTX_SUCCESS) throw std::runtime_error("ktxTexture2_CreateFromMemory failed: " + std::to_string(res));

	if (ktxTexture2_NeedsTranscoding(ktxTex))
	{
		res = ktxTexture2_TranscodeBasis(ktxTex, KTX_TTF_BC7_RGBA, 0);
		if (res != KTX_SUCCESS)
		{
			ktxTexture_Destroy(ktxTexture(ktxTex));
			throw std::runtime_error("ktxTexture2_TranscodeBasis failed: " + std::to_string(res));
		}
		// libktx sets vkFormat to BC7_UNORM_BLOCK after transcoding — promote to sRGB variant if needed
		if (isSrgb) ktxTex->vkFormat = VK_FORMAT_BC7_SRGB_BLOCK;
	}

	vk::Format vkFmt = static_cast<vk::Format>(ktxTex->vkFormat);
	uint32_t mipLevels = ktxTex->numLevels;
	uint32_t width = ktxTex->baseWidth;
	uint32_t height = ktxTex->baseHeight;

	// Create VkImage — pre-computed mipmaps, no eTransferSrc needed
	VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent = {width, height, 1};
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = static_cast<VkFormat>(vkFmt);
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	VmaAllocationCreateInfo vmaAllocInfo{};
	vmaAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	VkImage imageRaw;
	VmaAllocation vmaAlloc;
	if (vmaCreateImage(allocator, &imageInfo, &vmaAllocInfo, &imageRaw, &vmaAlloc, nullptr) != VK_SUCCESS)
	{
		ktxTexture_Destroy(ktxTexture(ktxTex));
		throw std::runtime_error("Failed to create VkImage for KTX2 texture");
	}
	texture.textureImage = vk::Image(imageRaw);
	texture.textureImageAllocation = vmaAlloc;
	texture.width = width;
	texture.height = height;
	texture.format = vkFmt;
	texture.tiling = vk::ImageTiling::eOptimal;
	texture.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
	texture.memoryUsage = VMA_MEMORY_USAGE_AUTO;
	texture.mipLevels = mipLevels;

	// Staging buffer — copy the full KTX data blob (all mip levels contiguous)
	ktx_size_t totalSize = ktxTexture_GetDataSize(ktxTexture(ktxTex));
	ktx_uint8_t* dataPtr = ktxTexture_GetData(ktxTexture(ktxTex));

	VkBufferCreateInfo bufInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	bufInfo.size = totalSize;
	bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	VmaAllocationCreateInfo stagingInfo{};
	stagingInfo.usage = VMA_MEMORY_USAGE_AUTO;
	stagingInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
	VkBuffer stagingBuf;
	VmaAllocation stagingAlloc;
	VmaAllocationInfo stagingAllocInfo;
	if (vmaCreateBuffer(allocator, &bufInfo, &stagingInfo, &stagingBuf, &stagingAlloc, &stagingAllocInfo) != VK_SUCCESS)
	{
		ktxTexture_Destroy(ktxTexture(ktxTex));
		throw std::runtime_error("Failed to create staging buffer for KTX2 texture");
	}
	memcpy(stagingAllocInfo.pMappedData, dataPtr, totalSize);

	auto cmd = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

	// Transition all mip levels: undefined -> transfer dst
	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = vk::ImageLayout::eUndefined;
	barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = texture.textureImage;
	barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0, 1};
	barrier.srcAccessMask = {};
	barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
	cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {},
	                    barrier);

	// One VkBufferImageCopy per mip level
	std::vector<vk::BufferImageCopy> regions;
	regions.reserve(mipLevels);
	for (uint32_t mip = 0; mip < mipLevels; mip++)
	{
		ktx_size_t offset;
		ktxTexture_GetImageOffset(ktxTexture(ktxTex), mip, 0, 0, &offset);

		vk::BufferImageCopy region;
		region.bufferOffset = offset;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource = {vk::ImageAspectFlagBits::eColor, mip, 0, 1};
		region.imageOffset = vk::Offset3D{0, 0, 0};
		region.imageExtent = vk::Extent3D{std::max(1u, width >> mip), std::max(1u, height >> mip), 1};
		regions.push_back(region);
	}
	cmd.copyBufferToImage(stagingBuf, texture.textureImage, vk::ImageLayout::eTransferDstOptimal, regions);

	// Transition all mip levels: transfer dst -> shader read only
	barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
	barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
	barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
	cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {},
	                    barrier);

	VulkanUtils::endSingleTimeCommands(cmd, vulkanDevice);
	vmaDestroyBuffer(allocator, stagingBuf, stagingAlloc);
	ktxTexture_Destroy(ktxTexture(ktxTex));
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
