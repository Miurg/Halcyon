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

	// Otherwise isTextureLoaded would later hand out this destroyed texture.
	for (auto it = texturePaths.begin(); it != texturePaths.end();)
	{
		if (it->second.id == handle.id)
			it = texturePaths.erase(it);
		else
			++it;
	}

	Texture& texture = textures[handle.id];
	destroySampler(texture.samplerHandle);
	texture.samplerHandle = {};
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
	freeSampler(live.samplerHandle, frameNumber);
	live.samplerHandle = {};
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
	if (!_freeSamplerSlots.empty())
	{
		int slot = _freeSamplerSlots.back();
		_freeSamplerSlots.pop_back();
		samplers[slot] = nullptr;
		return SamplerHandle{slot};
	}
	samplers.push_back(nullptr);
	return SamplerHandle{static_cast<int>(samplers.size() - 1)};
}

void TextureManager::createSampler(SamplerHandle handle, const SamplerDesc& desc, uint32_t mipLevels)
{
	samplers[handle.id] = VulkanUtils::createSampler(vulkanDevice, desc, mipLevels);
}

SamplerHandle TextureManager::createSampler(Texture& texture, const SamplerDesc& desc)
{
	SamplerHandle handle = allocateSamplerSlot();
	createSampler(handle, desc, texture.mipLevels);
	texture.samplerHandle = handle;
	return handle;
}

vk::Sampler TextureManager::getSampler(SamplerHandle handle)
{
	return samplers[handle.id];
}

void TextureManager::destroySampler(SamplerHandle handle)
{
	if (handle.id < 0 || handle.id >= static_cast<int>(samplers.size())) return;

	if (samplers[handle.id])
	{
		(*vulkanDevice.device).destroySampler(samplers[handle.id]);
		samplers[handle.id] = nullptr;
	}
	_freeSamplerSlots.push_back(handle.id);
}

void TextureManager::freeSampler(SamplerHandle handle, uint64_t frameNumber)
{
	if (handle.id < 0 || handle.id >= static_cast<int>(samplers.size())) return;

	_pendingSamplerFrees.push_back({handle.id, frameNumber + MAX_FRAMES_IN_FLIGHT});
}

void TextureManager::collectSamplerFrees(uint64_t frameNumber)
{
	for (auto it = _pendingSamplerFrees.begin(); it != _pendingSamplerFrees.end();)
	{
		if (it->retireFrame <= frameNumber)
		{
			if (samplers[it->slot])
			{
				(*vulkanDevice.device).destroySampler(samplers[it->slot]);
				samplers[it->slot] = nullptr;
			}
			_freeSamplerSlots.push_back(it->slot);
			it = _pendingSamplerFrees.erase(it);
		}
		else
			++it;
	}
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

MaterialStructure& TextureManager::getMaterial(int slot)
{
	return materials[slot];
}
