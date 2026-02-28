#include "TextureManager.hpp"
#include "DescriptorManager.hpp"
#include "BufferManager.hpp"
#include "../../Resources/Factories/TextureUploader.hpp"
#include "../../Factories/CommandBufferFactory.hpp"
#include "../../PipelineHandler.hpp"
#include "../../VulkanUtils.hpp"
#include <random>
#include <cmath>

TextureManager::TextureManager(VulkanDevice& vulkanDevice, VmaAllocator allocator)
    : vulkanDevice(vulkanDevice), allocator(allocator)
{
}

TextureManager::~TextureManager()
{
	for (auto& texture : textures)
	{
		if (texture.textureSampler)
		{
			(*vulkanDevice.device).destroySampler(texture.textureSampler);
		}

		if (texture.textureImageView)
		{
			(*vulkanDevice.device).destroyImageView(texture.textureImageView);
		}

		if (texture.textureImage)
		{
			vmaDestroyImage(allocator, texture.textureImage, texture.textureImageAllocation);
		}
	}
}

TextureHandle TextureManager::createDepthImage(uint32_t resolutionWidth, uint32_t resolutionHeight)
{
	textures.push_back(Texture());
	Texture& texture = textures.back();
	vk::Format depthFormat = findBestFormat();

	TextureManager::createImage(resolutionWidth, resolutionHeight, depthFormat, vk::ImageTiling::eOptimal,
	                            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
	                            VMA_MEMORY_USAGE_AUTO, texture);
	TextureManager::createImageView(texture, depthFormat, vk::ImageAspectFlagBits::eDepth);
	TextureManager::createTextureSampler(texture);
	return TextureHandle{static_cast<int>(textures.size() - 1)};
}

TextureHandle TextureManager::createOffscreenImage(uint32_t resolutionWidth, uint32_t resolutionHeight,
                                                   vk::Format offscreenFormat)
{
	textures.push_back(Texture());
	Texture& texture = textures.back();

	TextureManager::createImage(resolutionWidth, resolutionHeight, offscreenFormat, vk::ImageTiling::eOptimal,
	                            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
	                            VMA_MEMORY_USAGE_AUTO, texture);
	TextureManager::createImageView(texture, offscreenFormat, vk::ImageAspectFlagBits::eColor);
	TextureManager::createShadowSampler(texture);
	return TextureHandle{static_cast<int>(textures.size() - 1)};
}

void TextureManager::createOffscreenSampler(Texture& texture)
{
	vk::SamplerCreateInfo samplerInfo{};
	samplerInfo.magFilter = vk::Filter::eLinear;
	samplerInfo.minFilter = vk::Filter::eLinear;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.anisotropyEnable = vk::False;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
	samplerInfo.unnormalizedCoordinates = vk::False;
	samplerInfo.compareEnable = vk::False;
	samplerInfo.compareOp = vk::CompareOp::eAlways;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 1.0f;

	texture.textureSampler = (*vulkanDevice.device).createSampler(samplerInfo);
}

TextureHandle TextureManager::createShadowMap(uint32_t shadowResolutionX, uint32_t shadowResolutionY)
{
	textures.push_back(Texture());
	Texture& texture = textures.back();
	vk::Format shadowFormat = findBestFormat();

	TextureManager::createImage(shadowResolutionX, shadowResolutionY, shadowFormat, vk::ImageTiling::eOptimal,
	                            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
	                            VMA_MEMORY_USAGE_AUTO, texture);
	TextureManager::createImageView(texture, shadowFormat, vk::ImageAspectFlagBits::eDepth);
	TextureManager::createShadowSampler(texture);
	return TextureHandle{static_cast<int>(textures.size() - 1)};
}

TextureHandle TextureManager::createSsaoNoiseTexture()
{
	// 4x4 RGBA8
	std::mt19937 rng(42);
	std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
	unsigned char pixels[4 * 4 * 4];
	for (int i = 0; i < 16; i++)
	{
		float x = dist(rng);
		float y = dist(rng);
		float len = std::sqrt(x * x + y * y);
		if (len > 0.0001f)
		{
			x /= len;
			y /= len;
		}
		pixels[i * 4 + 0] = static_cast<unsigned char>((x * 0.5f + 0.5f) * 255.0f);
		pixels[i * 4 + 1] = static_cast<unsigned char>((y * 0.5f + 0.5f) * 255.0f);
		pixels[i * 4 + 2] = 0;
		pixels[i * 4 + 3] = 255;
	}

	textures.push_back(Texture());
	Texture& texture = textures.back();
	createImage(4, 4, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal,
	            vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, VMA_MEMORY_USAGE_AUTO, texture);
	TextureUploader::uploadTextureFromBuffer(pixels, 4, 4, texture, allocator, vulkanDevice);
	createImageView(texture, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);

	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = vk::Filter::eNearest;
	samplerInfo.minFilter = vk::Filter::eNearest;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
	texture.textureSampler = (*vulkanDevice.device).createSampler(samplerInfo);

	return TextureHandle{static_cast<int>(textures.size() - 1)};
}

void TextureManager::createShadowSampler(Texture& texture)
{
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = vk::Filter::eLinear;
	samplerInfo.minFilter = vk::Filter::eLinear;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;

	samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToBorder;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToBorder;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToBorder;
	samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueBlack;

	samplerInfo.compareEnable = vk::True;
	samplerInfo.compareOp = vk::CompareOp::eGreater;

	texture.textureSampler = (*vulkanDevice.device).createSampler(samplerInfo);
}

vk::Format TextureManager::findBestFormat()
{
	return findBestSupportedFormat({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
	                               vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

vk::Format TextureManager::findBestSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
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

TextureHandle TextureManager::generateTextureData(const char texturePath[MAX_PATH_LEN], int texWidth, int texHeight,
                                                  const unsigned char* pixels,
                                                  BindlessTextureDSetComponent& dSetComponent,
                                                  DescriptorManager& dManager, vk::Format format)
{
	if (isTextureLoaded(texturePath))
	{
		return texturePaths.find(texturePath)->second;
	}
	if (!pixels)
	{
		throw std::runtime_error("pixels data is null!");
	}
	if (texWidth <= 0 || texHeight <= 0)
	{
		throw std::runtime_error("Invalid texture dimensions!");
	}
	textures.push_back(Texture());
	Texture& texture = textures.back();

	TextureManager::createImage(texWidth, texHeight, format, vk::ImageTiling::eOptimal,
	                            vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
	                            VMA_MEMORY_USAGE_AUTO, texture);
	TextureUploader::uploadTextureFromBuffer(pixels, texWidth, texHeight, texture, allocator, vulkanDevice);
	TextureManager::createImageView(texture, format, vk::ImageAspectFlagBits::eColor);
	TextureManager::createTextureSampler(texture);

	TextureHandle handle{static_cast<int>(textures.size() - 1)};
	texturePaths[std::string(texturePath)] = handle;
	dManager.updateBindlessTextureSet(texture.textureImageView, texture.textureSampler, dSetComponent, handle.id);
	return handle;
}

bool TextureManager::isTextureLoaded(const char texturePath[MAX_PATH_LEN])
{
	return texturePaths.find(texturePath) != texturePaths.end();
}

void TextureManager::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
                                 vk::ImageUsageFlags usage, VmaMemoryUsage memoryUsage, Texture& texture)
{
	if (width == 0 || height == 0)
	{
		throw std::runtime_error("failed to create VMA image: invalid image dimensions (width or height is 0)!");
	}
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

	texture.width = width;
	texture.height = height;
	texture.format = format;
	texture.tiling = tiling;
	texture.usage = usage;
	texture.memoryUsage = memoryUsage;
}

int TextureManager::emplaceMaterials(BindlessTextureDSetComponent& dSetComponent, MaterialStructure materialStr,
                                     BufferManager& bManager)
{
	materials.push_back(materialStr);
	bManager.writeToBuffer(dSetComponent.materialBuffer, 0, materials.size() - 1, materialStr);
	return materials.size() - 1;
}

TextureHandle TextureManager::createCubemapImage(uint32_t width, uint32_t height, vk::Format format, uint32_t mipLevels)
{
	textures.push_back(Texture());
	Texture& texture = textures.back();

	texture.width = width;
	texture.height = height;
	texture.format = format;
	texture.tiling = vk::ImageTiling::eOptimal;
	texture.usage =
	    vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eStorage;
	texture.memoryUsage = VMA_MEMORY_USAGE_AUTO;
	texture.layerCount = 6;
	texture.mipLevels = mipLevels;
	texture.imageCreateFlags = vk::ImageCreateFlagBits::eCubeCompatible;

	VkImageCreateInfo imageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 6;
	imageInfo.format = static_cast<VkFormat>(format);
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = static_cast<VkImageUsageFlags>(texture.usage);
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = texture.memoryUsage;

	VkImage textureImageRaw;
	VkResult res =
	    vmaCreateImage(allocator, &imageInfo, &allocInfo, &textureImageRaw, &texture.textureImageAllocation, nullptr);
	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create VMA cubemap image!");
	}
	texture.textureImage = vk::Image(textureImageRaw);

	return TextureHandle{static_cast<int>(textures.size() - 1)};
}

void TextureManager::createImageView(Texture& texture, vk::Format format, vk::ImageAspectFlags aspectFlags)
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
	texture.aspectFlags = aspectFlags;
}

void TextureManager::createCubemapImageView(Texture& texture, vk::Format format, vk::ImageAspectFlags aspectFlags)
{
	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = texture.textureImage;
	viewInfo.viewType = vk::ImageViewType::eCube;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = texture.mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 6;

	texture.textureImageView = (*vulkanDevice.device).createImageView(viewInfo);
	texture.aspectFlags = aspectFlags;
}

void TextureManager::createTextureSampler(Texture& texture)
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

void TextureManager::createCubemapSampler(Texture& texture)
{
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = vk::Filter::eLinear;
	samplerInfo.minFilter = vk::Filter::eLinear;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.anisotropyEnable = vk::True;
	vk::PhysicalDeviceProperties properties = vulkanDevice.physicalDevice.getProperties();
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.compareOp = vk::CompareOp::eNever;
	samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<float>(texture.mipLevels);

	texture.textureSampler = (*vulkanDevice.device).createSampler(samplerInfo);
}

void TextureManager::resizeTexture(TextureHandle handle, uint32_t newWidth, uint32_t newHeight)
{
	if (handle.id < 0 || handle.id >= textures.size())
	{
		throw std::runtime_error("Invalid texture handle for resizing!");
	}

	Texture& texture = textures[handle.id];

	if (newWidth == 0 || newHeight == 0)
	{
		throw std::runtime_error("Cannot resize texture to 0 width or height!");
	}

	if (texture.width == newWidth && texture.height == newHeight)
	{
		return;
	}

	if (texture.textureImageView)
	{
		(*vulkanDevice.device).destroyImageView(texture.textureImageView);
	}
	if (texture.textureImage)
	{
		vmaDestroyImage(allocator, texture.textureImage, texture.textureImageAllocation);
	}

	TextureManager::createImage(newWidth, newHeight, texture.format, texture.tiling, texture.usage, texture.memoryUsage,
	                            texture);
	TextureManager::createImageView(texture, texture.format, texture.aspectFlags);
}

TextureHandle TextureManager::generateCubemapFromHdr(TextureHandle hdrTexture, 
                                                     PipelineHandler& pHandler, DescriptorManager& dManager,
                                                     BindlessTextureDSetComponent& dSetComponent)
{
	// Create Cubemap
	TextureHandle cubemapHandle = createCubemapImage(1024, 1024, vk::Format::eR32G32B32A32Sfloat, 1);
	Texture& cubemapTexture = textures[cubemapHandle.id];
	createCubemapImageView(cubemapTexture, vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor);
	createCubemapSampler(cubemapTexture);

	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = cubemapTexture.textureImage;
	viewInfo.viewType = vk::ImageViewType::e2DArray;
	viewInfo.format = vk::Format::eR32G32B32A32Sfloat;
	viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 6;
	vk::raii::ImageView storageImageView(vulkanDevice.device, viewInfo);

	dManager.updateCubemapDescriptors(dSetComponent, cubemapTexture.textureImageView, cubemapTexture.textureSampler,
	                                  *storageImageView);

	// Convert Equirectangular to Cubemap
	auto cmd = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

	VulkanUtils::transitionImageLayout(cmd, cubemapTexture.textureImage, vk::ImageLayout::eUndefined,
	                                vk::ImageLayout::eGeneral, {}, vk::AccessFlagBits2::eShaderWrite,
	                                vk::PipelineStageFlagBits2::eTopOfPipe, vk::PipelineStageFlagBits2::eComputeShader,
	                                vk::ImageAspectFlagBits::eColor, 6, 1);

	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pHandler.equirectToCubePipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pHandler.equirectToCubePipelineLayout, 0,
	                       dManager.descriptorSets[dSetComponent.bindlessTextureSet.id][0], nullptr);

	uint32_t pushConstants = hdrTexture.id;
	cmd.pushConstants<uint32_t>(*pHandler.equirectToCubePipelineLayout, vk::ShaderStageFlagBits::eCompute, 0,
	                            pushConstants);

	cmd.dispatch(1024 / 8, 1024 / 8, 6);

	VulkanUtils::transitionImageLayout(cmd, cubemapTexture.textureImage, vk::ImageLayout::eGeneral,
	                                vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits2::eShaderWrite,
	                                vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eComputeShader,
	                                vk::PipelineStageFlagBits2::eFragmentShader, vk::ImageAspectFlagBits::eColor, 6, 1);

	VulkanUtils::endSingleTimeCommands(cmd, vulkanDevice);

	return cubemapHandle;
}