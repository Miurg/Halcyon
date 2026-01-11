#include "BufferManager.hpp"
#include "../Factories/LoadFileFactory.hpp"

void BufferManager::bindShadowMap(int bufferIndex, vk::ImageView imageView, vk::Sampler sampler)
{
	for (size_t i = 0; i < buffers[bufferIndex].descriptorSet.size(); i++)
	{
		vk::DescriptorImageInfo imageInfo;
		imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		imageInfo.imageView = imageView;
		imageInfo.sampler = sampler;

		vk::WriteDescriptorSet descriptorWrite;
		descriptorWrite.dstSet = buffers[bufferIndex].descriptorSet[i];
		descriptorWrite.dstBinding = 1;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &imageInfo;

		vulkanDevice.device.updateDescriptorSets(descriptorWrite, {});
	}
}

int BufferManager::createShadowMap(uint32_t shadowResolutionX, uint32_t shadowResolutionY)
{
	textures.push_back(Texture());
	Texture& texture = textures.back();
	vk::Format shadowFormat = findBestFormat();

	BufferManager::createImage(shadowResolutionX, shadowResolutionY, shadowFormat, vk::ImageTiling::eOptimal,
	                           vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
	                           VMA_MEMORY_USAGE_AUTO, texture);
	BufferManager::createImageView(texture, shadowFormat, vk::ImageAspectFlagBits::eDepth);
	BufferManager::createShadowSampler(texture);
	return textures.size() - 1;
}

void BufferManager::createShadowSampler(Texture& texture)
{
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = vk::Filter::eLinear;
	samplerInfo.minFilter = vk::Filter::eLinear;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;

	samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToBorder;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToBorder;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToBorder;
	samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;

	samplerInfo.compareEnable = vk::True;
	samplerInfo.compareOp = vk::CompareOp::eLess;

	texture.textureSampler = (*vulkanDevice.device).createSampler(samplerInfo);
}

vk::Format BufferManager::findBestFormat()
{
	return findBestSupportedFormat({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
	                               vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

vk::Format BufferManager::findBestSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
                                                  vk::FormatFeatureFlags features)
{
	for (const auto format : candidates)
	{
		vk::FormatProperties props = vulkanDevice.physicalDevice.getFormatProperties(format);
		if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

int BufferManager::generateTextureData(const char texturePath[MAX_PATH_LEN], vk::Format format,
                                       vk::ImageAspectFlags aspectFlags)
{
	if (isTextureLoaded(texturePath))
	{
		return texturePaths[std::string(texturePath)];
	}
	textures.push_back(Texture());
	Texture& texture = textures.back();

	int texWidth, texHeight, texChannels;
	auto sizes = LoadFileFactory::getSizesFromFileTexture(texturePath);
	BufferManager::createImage(
	    std::get<0>(sizes), std::get<1>(sizes), vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal,
	    vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, VMA_MEMORY_USAGE_AUTO, texture);
	LoadFileFactory::uploadTextureFromFile(texturePath, texture, allocator, vulkanDevice);
	BufferManager::createImageView(texture, format, aspectFlags);
	BufferManager::createTextureSampler(texture);

	int numberTexture;
	numberTexture = textures.size() - 1;
	texturePaths[std::string(texturePath)] = numberTexture;
	updateBindlessTextureSet(numberTexture);
	return numberTexture;
}

bool BufferManager::isTextureLoaded(const char texturePath[MAX_PATH_LEN])
{
	return texturePaths.find(texturePath) != texturePaths.end();
}

void BufferManager::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
                                vk::ImageUsageFlags usage, VmaMemoryUsage memoryUsage, Texture& texture)
{
	VkImageCreateInfo imageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = static_cast<VkFormat>(format);
	imageInfo.tiling = static_cast<VkImageTiling>(tiling);
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = static_cast<VkImageUsageFlags>(usage);
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = memoryUsage;

	VkImage textureImageRaw;
	VkResult res =
	    vmaCreateImage(allocator, &imageInfo, &allocInfo, &textureImageRaw, &texture.textureImageAllocation, nullptr);
	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create VMA image!");
	}
	texture.textureImage = vk::Image(textureImageRaw);
}

void BufferManager::createImageView(Texture& texture, vk::Format format, vk::ImageAspectFlags aspectFlags)
{
	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = texture.textureImage;
	viewInfo.viewType = vk::ImageViewType::e2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	texture.textureImageView = (*vulkanDevice.device).createImageView(viewInfo);
}

void BufferManager::createTextureSampler(Texture& texture)
{
	vk::PhysicalDeviceProperties properties = vulkanDevice.physicalDevice.getProperties();
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = vk::Filter::eLinear;
	samplerInfo.minFilter = vk::Filter::eLinear;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;

	samplerInfo.anisotropyEnable = vk::True;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.compareOp = vk::CompareOp::eAlways;
	samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;

	texture.textureSampler = (*vulkanDevice.device).createSampler(samplerInfo);
}

void BufferManager::updateBindlessTextureSet(int textureNumber)
{
	vk::DescriptorImageInfo imageInfo;
	imageInfo.sampler = textureSampler;
	imageInfo.imageView = textures[textureNumber].textureImageView;
	imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

	vk::WriteDescriptorSet descriptorWrite;
	descriptorWrite.dstSet = bindlessTextureSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = textureNumber;
	descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	vulkanDevice.device.updateDescriptorSets(descriptorWrite, {});
}