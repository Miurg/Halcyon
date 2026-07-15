#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "GraphicsCore/Resources/Factories/TextureUploader.hpp"
#include "GraphicsCore/VulkanUtils.hpp"
#include "GraphicsCore/Resources/Managers/Bindings.hpp"
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
		destroyTextureResources(texture);
	}
	for (auto& pending : _pendingFrees)
	{
		destroyTextureResources(pending.texture);
	}
}

void TextureManager::destroyTextureResources(Texture& texture)
{
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
		texture.textureImage = nullptr;
		texture.textureImageAllocation = nullptr;
	}
}

void TextureManager::destroyTexture(TextureHandle handle)
{
	if (handle.id < 0 || handle.id >= static_cast<int>(textures.size())) return;

	// Otherwise isTextureLoaded would later hand out this destroyed texture.
	for (auto it = texturePaths.begin(); it != texturePaths.end();)
	{
		if (it->second.id == handle.id)
			it = texturePaths.erase(it);
		else
			++it;
	}

	destroyTextureResources(textures[handle.id]);
	_freeTextureSlots.push_back(handle.id);
}

int TextureManager::allocateTextureSlot()
{
	if (!_freeTextureSlots.empty())
	{
		int slot = _freeTextureSlots.back();
		_freeTextureSlots.pop_back();
		textures[slot] = Texture();
		return slot;
	}
	textures.push_back(Texture());
	return static_cast<int>(textures.size() - 1);
}

void TextureManager::freeTexture(TextureHandle handle, uint64_t frameNumber)
{
	if (handle.id < 0 || handle.id >= static_cast<int>(textures.size())) return;

	// Otherwise isTextureLoaded would later hand out this freed texture.
	for (auto it = texturePaths.begin(); it != texturePaths.end();)
	{
		if (it->second.id == handle.id)
			it = texturePaths.erase(it);
		else
			++it;
	}

	Texture& live = textures[handle.id];
	_pendingFrees.push_back({live, handle.id, frameNumber + MAX_FRAMES_IN_FLIGHT});

	// Nulled so slot reuse and the destructor don't double-free these handles.
	live.textureImage = nullptr;
	live.textureImageView = nullptr;
	live.textureSampler = nullptr;
	live.textureImageAllocation = nullptr;
}

void TextureManager::collectTextureFrees(uint64_t frameNumber)
{
	for (auto it = _pendingFrees.begin(); it != _pendingFrees.end();)
	{
		if (it->retireFrame <= frameNumber)
		{
			destroyTextureResources(it->texture);
			_freeTextureSlots.push_back(it->slot);
			it = _pendingFrees.erase(it);
		}
		else
			++it;
	}
}

TextureHandle TextureManager::createDepthImage(uint32_t resolutionWidth, uint32_t resolutionHeight)
{
	int slot = allocateTextureSlot();
	Texture& texture = textures[slot];
	vk::Format depthFormat = findBestFormat();

	TextureManager::createImage(resolutionWidth, resolutionHeight, depthFormat, vk::ImageTiling::eOptimal,
	                            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
	                            VMA_MEMORY_USAGE_AUTO, texture);
	TextureManager::createImageView(texture, depthFormat, vk::ImageAspectFlagBits::eDepth);
	TextureManager::createTextureSampler(texture);
	return TextureHandle{slot};
}

TextureHandle TextureManager::createOffscreenImage(uint32_t resolutionWidth, uint32_t resolutionHeight,
                                                   vk::Format offscreenFormat)
{
	int slot = allocateTextureSlot();
	Texture& texture = textures[slot];

	TextureManager::createImage(resolutionWidth, resolutionHeight, offscreenFormat, vk::ImageTiling::eOptimal,
	                            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
	                            VMA_MEMORY_USAGE_AUTO, texture);
	TextureManager::createImageView(texture, offscreenFormat, vk::ImageAspectFlagBits::eColor);
	TextureManager::createShadowSampler(texture);
	return TextureHandle{slot};
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
	int slot = allocateTextureSlot();
	Texture& texture = textures[slot];
	vk::Format shadowFormat = findBestFormat();

	TextureManager::createImage(shadowResolutionX, shadowResolutionY, shadowFormat, vk::ImageTiling::eOptimal,
	                            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
	                            VMA_MEMORY_USAGE_AUTO, texture);
	TextureManager::createImageView(texture, shadowFormat, vk::ImageAspectFlagBits::eDepth);
	TextureManager::createShadowSampler(texture);
	return TextureHandle{slot};
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
	int slot = allocateTextureSlot();
	Texture& texture = textures[slot];

	uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
	TextureManager::createImage(texWidth, texHeight, format, vk::ImageTiling::eOptimal,
	                            vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst |
	                                vk::ImageUsageFlagBits::eSampled,
	                            VMA_MEMORY_USAGE_AUTO, texture, mipLevels);
	TextureUploader::uploadTextureFromBuffer(pixels, texWidth, texHeight, texture, allocator, vulkanDevice);
	TextureManager::createImageView(texture, format, vk::ImageAspectFlagBits::eColor);
	TextureManager::createTextureSampler(texture);

	TextureHandle handle{slot};
	texturePaths[std::string(texturePath)] = handle;
	dManager.update(dSetComponent.bindlessTextureSet, Bindings::Textures::Array, 0,
	                vk::DescriptorType::eCombinedImageSampler, texture.textureImageView, texture.textureSampler,
	                vk::ImageLayout::eShaderReadOnlyOptimal, static_cast<uint32_t>(handle.id));
	return handle;
}

TextureHandle TextureManager::generateTextureDataFromKtx(const char texturePath[MAX_PATH_LEN],
                                                         const unsigned char* ktxData, size_t dataSize,
                                                         BindlessTextureDSetComponent& dSetComponent,
                                                         DescriptorManager& dManager, bool isSrgb)
{
	int slot = allocateTextureSlot();
	Texture& texture = textures[slot];

	TextureUploader::uploadKtxTextureData(ktxData, dataSize, texture, isSrgb, allocator, vulkanDevice);
	createImageView(texture, texture.format, vk::ImageAspectFlagBits::eColor);
	createTextureSampler(texture); // maxLod uses texture.mipLevels set inside uploadKtxTextureData

	TextureHandle handle{slot};
	texturePaths[std::string(texturePath)] = handle;
	dManager.update(dSetComponent.bindlessTextureSet, Bindings::Textures::Array, 0,
	                vk::DescriptorType::eCombinedImageSampler, texture.textureImageView, texture.textureSampler,
	                vk::ImageLayout::eShaderReadOnlyOptimal, static_cast<uint32_t>(handle.id));
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
	int slot;
	if (!_freeMaterialSlots.empty())
	{
		slot = _freeMaterialSlots.back();
		_freeMaterialSlots.pop_back();
		materials[slot] = materialStr;
	}
	else
	{
		materials.push_back(materialStr);
		slot = static_cast<int>(materials.size() - 1);
	}
	bManager.writeToBuffer(dSetComponent.materialBuffer, 0, slot, materialStr);
	return slot;
}

void TextureManager::freeMaterial(int slot, uint64_t frameNumber)
{
	_pendingMaterialFrees.push_back({slot, frameNumber + MAX_FRAMES_IN_FLIGHT});
}

void TextureManager::collectMaterialFrees(uint64_t frameNumber)
{
	for (auto it = _pendingMaterialFrees.begin(); it != _pendingMaterialFrees.end();)
	{
		if (it->retireFrame <= frameNumber)
		{
			_freeMaterialSlots.push_back(it->slot);
			it = _pendingMaterialFrees.erase(it);
		}
		else
			++it;
	}
}

size_t TextureManager::freeTextureSlotCount() const
{
	return _freeTextureSlots.size();
}

size_t TextureManager::pendingTextureFreeCount() const
{
	return _pendingFrees.size();
}

size_t TextureManager::freeMaterialSlotCount() const
{
	return _freeMaterialSlots.size();
}

size_t TextureManager::pendingMaterialFreeCount() const
{
	return _pendingMaterialFrees.size();
}

TextureHandle TextureManager::createCubemapImage(uint32_t width, uint32_t height, vk::Format format,
                                                 vk::ImageUsageFlags usage, uint32_t mipLevels)
{
	int slot = allocateTextureSlot();
	Texture& texture = textures[slot];

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

	return TextureHandle{slot};
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

Texture& TextureManager::getTexture(TextureHandle handle)
{
	return textures[handle.id];
}

MaterialStructure& TextureManager::getMaterial(int slot)
{
	return materials[slot];
}
