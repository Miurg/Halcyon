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
	// TODO: mass refactor
	TextureManager(VulkanDevice& vulkanDevice, VmaAllocator allocator);
	~TextureManager();
	TextureHandle generateTextureData(const char texturePath[MAX_PATH_LEN], int texWidth, int texHeight,
	                                  const unsigned char* pixels, BindlessTextureDSetComponent& dSetComponent,
	                                  DescriptorManager& dManager, vk::Format format = vk::Format::eR8G8B8A8Srgb);
	TextureHandle generateTextureDataFromKtx(const char texturePath[MAX_PATH_LEN], const unsigned char* ktxData,
	                                         size_t dataSize, BindlessTextureDSetComponent& dSetComponent,
	                                         DescriptorManager& dManager, bool isSrgb);
	bool isTextureLoaded(const char texturePath[MAX_PATH_LEN]);
	void resizeTexture(TextureHandle handle, uint32_t newWidth, uint32_t newHeight);
	TextureHandle createDepthImage(uint32_t resolutionWidth, uint32_t resolutionHeight);
	TextureHandle createOffscreenImage(uint32_t resolutionWidth, uint32_t resolutionHeight, vk::Format offscreenFormat);
	TextureHandle createShadowMap(uint32_t shadowResolutionX, uint32_t shadowResolutionY);
	void createImageView(Texture& texture, vk::Format format, vk::ImageAspectFlags aspectFlags,
	                     vk::ImageViewType viewType = vk::ImageViewType::e2D);
	void createSampler(Texture& texture, const SamplerDesc& desc);
	vk::Format findBestFormat();
	vk::Format findBestSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
	                                   vk::FormatFeatureFlags features);
	void createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
	                 vk::ImageUsageFlags usage, VmaMemoryUsage memoryUsage, Texture& texture, uint32_t mipLevels = 1);
	TextureHandle createCubemapImage(uint32_t width, uint32_t height, vk::Format format,
	                                 vk::ImageUsageFlags usage, uint32_t mipLevels = 1);
	void destroyTexture(TextureHandle handle);
	int allocateTextureSlot();
	void freeTexture(TextureHandle handle, uint64_t frameNumber);
	void collectTextureFrees(uint64_t frameNumber);
	Texture& getTexture(TextureHandle handle);

	std::vector<Texture> textures;
	std::unordered_map<std::string, TextureHandle> texturePaths;

	int emplaceMaterials(BindlessTextureDSetComponent& dSetComponent, MaterialStructure materialMaps,
	                     BufferManager& bManager);
	void freeMaterial(int slot, uint64_t frameNumber);
	void collectMaterialFrees(uint64_t frameNumber);
	size_t freeTextureSlotCount() const;
	size_t pendingTextureFreeCount() const;
	size_t freeMaterialSlotCount() const;
	size_t pendingMaterialFreeCount() const;
	MaterialStructure& getMaterial(int slot);

	std::vector<MaterialStructure> materials;

private:
	VulkanDevice& vulkanDevice;
	VmaAllocator allocator = {};
	std::vector<int> _freeTextureSlots;

	struct PendingTextureFree
	{
		Texture texture;
		int slot;
		uint64_t retireFrame;
	};
	std::vector<PendingTextureFree> _pendingFrees;

	struct PendingMaterialFree
	{
		int slot;
		uint64_t retireFrame;
	};
	std::vector<PendingMaterialFree> _pendingMaterialFrees;
	std::vector<int> _freeMaterialSlots;

	void destroyTextureResources(Texture& texture);
};