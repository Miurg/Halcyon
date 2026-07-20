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
	for (auto& sampler : samplers)
	{
		if (sampler)
		{
			(*vulkanDevice.device).destroySampler(sampler);
		}
	}
}

void TextureManager::destroyTextureResources(Texture& texture)
{
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
	if (_textureRefCounts[handle.id] <= 0) return;
	if (--_textureRefCounts[handle.id] > 0) return;

	// Otherwise isTextureLoaded would later hand out this destroyed texture.
	for (auto it = texturePaths.begin(); it != texturePaths.end();)
	{
		if (it->second.id == handle.id)
			it = texturePaths.erase(it);
		else
			++it;
	}

	Texture& texture = textures[handle.id];
	destroyTextureResources(texture);
	_freeTextureSlots.push_back(handle.id);
}

int TextureManager::allocateTextureSlot()
{
	if (!_freeTextureSlots.empty())
	{
		int slot = _freeTextureSlots.back();
		_freeTextureSlots.pop_back();
		textures[slot] = Texture();
		_textureRefCounts[slot] = 1;
		return slot;
	}
	textures.push_back(Texture());
	_textureRefCounts.push_back(1);
	return static_cast<int>(textures.size() - 1);
}

void TextureManager::addTextureRef(TextureHandle handle)
{
	if (handle.id < 0 || handle.id >= static_cast<int>(textures.size())) return;

	_textureRefCounts[handle.id]++;
}

void TextureManager::freeTexture(TextureHandle handle, uint64_t frameNumber)
{
	if (handle.id < 0 || handle.id >= static_cast<int>(textures.size())) return;
	if (_textureRefCounts[handle.id] <= 0) return;
	if (--_textureRefCounts[handle.id] > 0) return;

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

bool TextureManager::isTextureLoaded(const char texturePath[MAX_PATH_LEN])
{
	return texturePaths.find(texturePath) != texturePaths.end();
}

void TextureManager::createImage(Texture& texture, const ImageDesc& desc)
{
	AllocatedImage allocated = VulkanUtils::createImage(allocator, desc);
	texture.textureImage = allocated.image;
	texture.textureImageAllocation = allocated.allocation;

	texture.width = desc.width;
	texture.height = desc.height;
	texture.format = desc.format;
	texture.tiling = desc.tiling;
	texture.usage = desc.usage;
	texture.memoryUsage = desc.memoryUsage;
	texture.mipLevels = desc.mipLevels;
	texture.layerCount = desc.layerCount;
	texture.imageCreateFlags = desc.flags;
}

size_t TextureManager::freeTextureSlotCount() const
{
	return _freeTextureSlots.size();
}

size_t TextureManager::pendingTextureFreeCount() const
{
	return _pendingFrees.size();
}

void TextureManager::createImageView(Texture& texture, vk::Format format, vk::ImageAspectFlags aspectFlags,
                                     vk::ImageViewType viewType)
{
	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = texture.textureImage;
	viewInfo.viewType = viewType;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = texture.mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = texture.layerCount;

	texture.textureImageView = (*vulkanDevice.device).createImageView(viewInfo);
	texture.aspectFlags = aspectFlags;
}

SamplerHandle TextureManager::allocateSamplerSlot()
{
	samplers.push_back(nullptr);
	return SamplerHandle{static_cast<int>(samplers.size() - 1)};
}

void TextureManager::createSampler(SamplerHandle handle, const SamplerDesc& desc)
{
	samplers[handle.id] = VulkanUtils::createSampler(vulkanDevice, desc);
}

SamplerHandle TextureManager::createSampler(Texture& texture, const SamplerDesc& desc)
{
	auto cached = _samplerCache.find(desc);
	if (cached != _samplerCache.end())
	{
		texture.samplerHandle = cached->second;
		return cached->second;
	}

	SamplerHandle handle = allocateSamplerSlot();
	createSampler(handle, desc);
	_samplerCache.emplace(desc, handle);
	texture.samplerHandle = handle;
	return handle;
}

vk::Sampler TextureManager::getSampler(SamplerHandle handle)
{
	return samplers[handle.id];
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

	ImageDesc desc;
	desc.width = newWidth;
	desc.height = newHeight;
	desc.format = texture.format;
	desc.usage = texture.usage;
	desc.mipLevels = texture.mipLevels;
	desc.layerCount = texture.layerCount;
	desc.flags = texture.imageCreateFlags;
	desc.tiling = texture.tiling;
	desc.memoryUsage = texture.memoryUsage;
	createImage(texture, desc);
	TextureManager::createImageView(texture, texture.format, texture.aspectFlags);
}

Texture& TextureManager::getTexture(TextureHandle handle)
{
	return textures[handle.id];
}
