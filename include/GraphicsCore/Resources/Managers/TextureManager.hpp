#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/VertexIndexBuffer.hpp"
#include <vector>
#include <unordered_map>
#include "GraphicsCore/Resources/Components/MeshInfoComponent.hpp"
#include "GraphicsCore/VulkanConst.hpp"
#include <vk_mem_alloc.h>
#include "GraphicsCore/Resources/Managers/Texture.hpp"
#include "GraphicsCore/SamplerDesc.hpp"
#include "GraphicsCore/ImageDesc.hpp"
#include "GraphicsCore/Resources/Managers/Buffer.hpp"
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include "GraphicsCore/VulkanDevice.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"
#include "GraphicsCore/Resources/ResourceStructures.hpp"

class BufferManager;
class DescriptorManager;
class CommandBufferFactory;
class PipelineFactory;
class PipelineHandler;

// Creates and caches textures (VMA-allocated). Deduplicates by file path.
class HALCYON_API TextureManager
{
public:
	TextureManager(VulkanDevice& vulkanDevice, VmaAllocator allocator);
	~TextureManager();
	bool isTextureLoaded(const char texturePath[MAX_PATH_LEN]) const;
	TextureHandle getTextureHandle(const char texturePath[MAX_PATH_LEN]) const;
	void registerTexturePath(const char texturePath[MAX_PATH_LEN], TextureHandle handle);
	void unregisterTexturePath(TextureHandle handle);
	void resizeTexture(TextureHandle handle, uint32_t newWidth, uint32_t newHeight);
	void createImageView(Texture& texture, vk::Format format, vk::ImageAspectFlags aspectFlags,
	                     vk::ImageViewType viewType = vk::ImageViewType::e2D);
	SamplerHandle allocateSamplerSlot();
	void createSampler(SamplerHandle handle, const SamplerDesc& desc);
	SamplerHandle createSampler(Texture& texture, const SamplerDesc& desc);
	vk::Sampler getSampler(SamplerHandle handle);
	vk::Format findBestFormat();
	vk::Format findBestSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
	                                   vk::FormatFeatureFlags features);
	void createImage(Texture& texture, const ImageDesc& desc);
	void destroyTexture(TextureHandle handle);
	int allocateTextureSlot();
	void addTextureRef(TextureHandle handle);
	bool releaseTextureRef(TextureHandle handle);
	void freeTexture(TextureHandle handle, uint64_t frameNumber);
	void collectTextureFrees(uint64_t frameNumber);
	Texture& getTexture(TextureHandle handle);

	size_t textureCount() const;
	size_t samplerCount() const;
	size_t freeTextureSlotCount() const;
	size_t pendingTextureFreeCount() const;

private:
	std::vector<Texture> textures;
	std::vector<vk::Sampler> samplers;
	std::unordered_map<std::string, TextureHandle> texturePaths;

	VulkanDevice& vulkanDevice;
	VmaAllocator allocator = {};
	std::vector<int> _freeTextureSlots;
	std::vector<int> _textureRefCounts;

	struct PendingTextureFree
	{
		Texture texture;
		int slot;
		uint64_t retireFrame;
	};
	std::vector<PendingTextureFree> _pendingFrees;

	std::unordered_map<SamplerDesc, SamplerHandle> _samplerCache;

	void destroyTextureResources(Texture& texture);
};