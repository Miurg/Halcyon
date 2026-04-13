#include "TextureManager.hpp"
#include "DescriptorManager.hpp"
#include "BufferManager.hpp"
#include "../../Resources/Factories/TextureUploader.hpp"
#include "../../VulkanUtils.hpp"
#include "Bindings.hpp"
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

void TextureManager::destroyTexture(TextureHandle handle)
{
	Texture& texture = textures[handle.id];

	if (texture.textureSampler)
	{
		(*vulkanDevice.device).destroySampler(texture.textureSampler);
		texture.textureSampler = nullptr;
	}

	if (texture.textureImageView)
	{
		(*vulkanDevice.device).destroyImageView(texture.textureImageView);
		texture.textureImageView = nullptr;
	}

	if (texture.textureImage)
	{
		vmaDestroyImage(allocator, texture.textureImage, texture.textureImageAllocation);
		texture.textureImage     = nullptr;
		texture.textureImageAllocation = nullptr;
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

	uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
	TextureManager::createImage(texWidth, texHeight, format, vk::ImageTiling::eOptimal,
	                            vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst |
	                                vk::ImageUsageFlagBits::eSampled,
	                            VMA_MEMORY_USAGE_AUTO, texture, mipLevels);
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
                                 vk::ImageUsageFlags usage, VmaMemoryUsage memoryUsage, Texture& texture,
                                 uint32_t mipLevels)
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
	imageInfo.mipLevels = mipLevels;
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
	texture.mipLevels = mipLevels;
}

int TextureManager::emplaceMaterials(BindlessTextureDSetComponent& dSetComponent, MaterialStructure materialStr,
                                     BufferManager& bManager)
{
	materials.push_back(materialStr);
	bManager.writeToBuffer(dSetComponent.materialBuffer, 0, materials.size() - 1, materialStr);
	return materials.size() - 1;
}

TextureHandle TextureManager::createCubemapImage(uint32_t width, uint32_t height, vk::Format format,
                                                 vk::ImageUsageFlags usage, uint32_t mipLevels)
{
	textures.push_back(Texture());
	Texture& texture = textures.back();

	texture.width = width;
	texture.height = height;
	texture.format = format;
	texture.tiling = vk::ImageTiling::eOptimal;
	texture.usage = usage;
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
	viewInfo.subresourceRange.levelCount = texture.mipLevels;
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
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<float>(texture.mipLevels);

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
                                                     DescriptorManager& dManager,
                                                     BindlessTextureDSetComponent& dSetComponent,
                                                     PipelineManager& pManager)
{
	// Create Cubemap
	TextureHandle cubemapHandle = createCubemapImage(
	    1024, 1024, vk::Format::eR32G32B32A32Sfloat,
	    vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eStorage);
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

	VulkanUtils::transitionImageLayout(
	    cmd, cubemapTexture.textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, {},
	    vk::AccessFlagBits2::eShaderWrite, vk::PipelineStageFlagBits2::eTopOfPipe,
	    vk::PipelineStageFlagBits2::eComputeShader, vk::ImageAspectFlagBits::eColor, 6, 1);

	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["equirect_to_cube"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["equirect_to_cube"].layout, 0,
	                       dManager.getSet(dSetComponent.bindlessTextureSet), nullptr);

	uint32_t pushConstants = hdrTexture.id;
	cmd.pushConstants<uint32_t>(*pManager.pipelines["equirect_to_cube"].layout, vk::ShaderStageFlagBits::eCompute, 0,
	                            pushConstants);

	cmd.dispatch(1024 / 8, 1024 / 8, 6); //TODO: Get rid of hardcoded resolution. Same in SH compute.

	VulkanUtils::transitionImageLayout(
	    cmd, cubemapTexture.textureImage, vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal,
	    vk::AccessFlagBits2::eShaderWrite, vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eComputeShader,
	    vk::PipelineStageFlagBits2::eFragmentShader, vk::ImageAspectFlagBits::eColor, 6, 1);

	VulkanUtils::endSingleTimeCommands(cmd, vulkanDevice);

	return cubemapHandle;
}

TextureHandle TextureManager::generatePrefilteredEnvMap(TextureHandle envCubemap,
                                                        DescriptorManager& dManager,
                                                        BindlessTextureDSetComponent& dSetComponent,
                                                        PipelineManager& pManager)
{
	const uint32_t prefilteredSize = 128;
	const uint32_t maxMipLevels = 5;

	TextureHandle prefilteredHandle = createCubemapImage(
	    prefilteredSize, prefilteredSize, vk::Format::eR32G32B32A32Sfloat,
	    vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eStorage,
	    maxMipLevels);
	Texture& prefilteredTexture = textures[prefilteredHandle.id];
	createCubemapImageView(prefilteredTexture, vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor);
	createCubemapSampler(prefilteredTexture);

	auto initCmd = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

	VulkanUtils::transitionImageLayout(
	    initCmd, prefilteredTexture.textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, {},
	    vk::AccessFlagBits2::eShaderWrite, vk::PipelineStageFlagBits2::eTopOfPipe,
	    vk::PipelineStageFlagBits2::eComputeShader, vk::ImageAspectFlagBits::eColor, 6, maxMipLevels);

	VulkanUtils::endSingleTimeCommands(initCmd, vulkanDevice);

	// Process each mip level in a separate command buffer so storage views stay alive
	for (uint32_t mip = 0; mip < maxMipLevels; mip++)
	{
		uint32_t mipWidth = prefilteredSize >> mip;
		uint32_t mipHeight = prefilteredSize >> mip;

		// Create a storage image view for this specific mip level
		vk::ImageViewCreateInfo viewInfo;
		viewInfo.image = prefilteredTexture.textureImage;
		viewInfo.viewType = vk::ImageViewType::e2DArray;
		viewInfo.format = vk::Format::eR32G32B32A32Sfloat;
		viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		viewInfo.subresourceRange.baseMipLevel = mip;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 6;
		vk::raii::ImageView mipStorageView(vulkanDevice.device, viewInfo);

		// Update cubemap descriptors: environment stays as sampler, output is this mip
		dManager.updateCubemapDescriptors(dSetComponent, textures[envCubemap.id].textureImageView,
		                                  textures[envCubemap.id].textureSampler, *mipStorageView);

		auto cmd = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

		cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["prefilter_env_map"].pipeline);
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["prefilter_env_map"].layout, 0,
		                       dManager.getSet(dSetComponent.bindlessTextureSet), nullptr);

		float roughness = static_cast<float>(mip) / static_cast<float>(maxMipLevels - 1);
		cmd.pushConstants<float>(*pManager.pipelines["prefilter_env_map"].layout, vk::ShaderStageFlagBits::eCompute, 0,
		                         roughness);

		uint32_t groupsX = std::max(1u, mipWidth / 8);
		uint32_t groupsY = std::max(1u, mipHeight / 8);
		cmd.dispatch(groupsX, groupsY, 6);

		VulkanUtils::endSingleTimeCommands(cmd, vulkanDevice);
	}

	auto finalCmd = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

	VulkanUtils::transitionImageLayout(
	    finalCmd, prefilteredTexture.textureImage, vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal,
	    vk::AccessFlagBits2::eShaderWrite, vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eComputeShader,
	    vk::PipelineStageFlagBits2::eFragmentShader, vk::ImageAspectFlagBits::eColor, 6, maxMipLevels);

	VulkanUtils::endSingleTimeCommands(finalCmd, vulkanDevice);

	return prefilteredHandle;
}

TextureHandle TextureManager::generateBrdfLut(DescriptorManager& dManager, BindlessTextureDSetComponent& dSetComponent,
                                              PipelineManager& pManager)
{
	const uint32_t brdfLutSize = 512;

	textures.push_back(Texture());
	Texture& brdfLutTexture = textures.back();

	createImage(brdfLutSize, brdfLutSize, vk::Format::eR32G32Sfloat, vk::ImageTiling::eOptimal,
	            vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage, VMA_MEMORY_USAGE_AUTO,
	            brdfLutTexture);
	createImageView(brdfLutTexture, vk::Format::eR32G32Sfloat, vk::ImageAspectFlagBits::eColor);

	// Create a sampler for the BRDF LUT
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = vk::Filter::eLinear;
	samplerInfo.minFilter = vk::Filter::eLinear;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
	brdfLutTexture.textureSampler = (*vulkanDevice.device).createSampler(samplerInfo);

	// Create a temporary storage image view for compute write
	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = brdfLutTexture.textureImage;
	viewInfo.viewType = vk::ImageViewType::e2D;
	viewInfo.format = vk::Format::eR32G32Sfloat;
	viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	vk::raii::ImageView storageView(vulkanDevice.device, viewInfo);

	// Temporarily update the CubemapStorage descriptor to point to our BRDF LUT output
	vk::DescriptorImageInfo storageImgInfo;
	storageImgInfo.sampler = nullptr;
	storageImgInfo.imageView = *storageView;
	storageImgInfo.imageLayout = vk::ImageLayout::eGeneral;

	vk::WriteDescriptorSet storageWrite;
	storageWrite.dstSet = dManager.getSet(dSetComponent.bindlessTextureSet);
	storageWrite.dstBinding = Bindings::Textures::CubemapStorage;
	storageWrite.dstArrayElement = 0;
	storageWrite.descriptorType = vk::DescriptorType::eStorageImage;
	storageWrite.descriptorCount = 1;
	storageWrite.pImageInfo = &storageImgInfo;
	vulkanDevice.device.updateDescriptorSets(storageWrite, {});

	auto cmd = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

	VulkanUtils::transitionImageLayout(
	    cmd, brdfLutTexture.textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, {},
	    vk::AccessFlagBits2::eShaderWrite, vk::PipelineStageFlagBits2::eTopOfPipe,
	    vk::PipelineStageFlagBits2::eComputeShader, vk::ImageAspectFlagBits::eColor, 1, 1);

	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["brdf_lut"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["brdf_lut"].layout, 0,
	                       dManager.getSet(dSetComponent.bindlessTextureSet), nullptr);

	cmd.dispatch(brdfLutSize / 8, brdfLutSize / 8, 1);

	VulkanUtils::transitionImageLayout(
	    cmd, brdfLutTexture.textureImage, vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal,
	    vk::AccessFlagBits2::eShaderWrite, vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eComputeShader,
	    vk::PipelineStageFlagBits2::eFragmentShader, vk::ImageAspectFlagBits::eColor, 1, 1);

	VulkanUtils::endSingleTimeCommands(cmd, vulkanDevice);

	return TextureHandle{static_cast<int>(textures.size() - 1)};
}