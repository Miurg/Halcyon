#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/VertexIndexBuffer.hpp"
#include <vector>
#include <optional>
#include <cstdint>
#include <unordered_map>
#include "GraphicsCore/Resources/Components/MeshInfoComponent.hpp"
#include "GraphicsCore/VulkanConst.hpp"
#include <vk_mem_alloc.h>
#include "GraphicsCore/Resources/Managers/Texture.hpp"
#include "GraphicsCore/Resources/Managers/Vertex.hpp"
#include "GraphicsCore/Resources/Managers/Buffer.hpp"
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include "GraphicsCore/VulkanDevice.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "GraphicsCore/Resources/Managers/MeshInfo.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"

struct HALCYON_API GeometryAllocation
{
	uint32_t vertexBase = 0;
	uint32_t vertexCount = 0;
	uint32_t indexBase = 0;
	uint32_t indexCount = 0;
	int bufferIndex = 0;
};

struct HALCYON_API RenderAsset
{
	GeometryAllocation allocation;
	std::vector<MeshHandle> meshes;
	std::vector<TextureHandle> textures;
	std::vector<MaterialHandle> materials;
	int refCount = 0;
};

// Stores loaded meshes and their GPU vertex/index buffers. Deduplicates by file path.
class HALCYON_API RenderAssetManager
{
public:
	RenderAssetManager(VulkanDevice& vulkanDevice, VmaAllocator allocator);
	~RenderAssetManager();
	bool isRenderAssetLoaded(const char path[MAX_PATH_LEN]) const;
	RenderAssetHandle getRenderAssetHandle(const char path[MAX_PATH_LEN]) const;
	void registerRenderAssetPath(const char path[MAX_PATH_LEN], RenderAssetHandle handle);
	void unregisterRenderAssetPath(RenderAssetHandle handle);

	std::optional<GeometryAllocation> allocateGeometry(int bufferIndex, uint32_t vertexCount, uint32_t indexCount);
	void uploadVertices(int bufferIndex, uint32_t vertexBase, const Vertex* data, uint32_t count);
	void uploadIndices(int bufferIndex, uint32_t indexBase, const uint32_t* data, uint32_t count);
	void freeGeometry(const GeometryAllocation& allocation, uint64_t frameNumber);
	void collectGeometryFrees(uint64_t frameNumber);
	void defragment(VertexIndexBuffer& buffer);

	MeshHandle allocateMeshSlot();
	RenderAssetHandle allocateRenderAssetSlot();
	void addRenderAssetRef(RenderAssetHandle handle);
	bool releaseRenderAssetRef(RenderAssetHandle handle);
	void freeMeshSlot(MeshHandle handle);
	void freeRenderAssetSlot(RenderAssetHandle handle);
	size_t meshCount() const;
	size_t renderAssetCount() const;
	size_t freeMeshSlotCount() const;
	size_t freeRenderAssetSlotCount() const;
	size_t pendingGeometryFreeCount() const;

	VertexIndexBuffer& getVertexIndexBuffer(int index);
	MeshInfo& getMesh(MeshHandle handle);
	RenderAsset& getRenderAsset(RenderAssetHandle handle);

private:
	std::vector<VertexIndexBuffer> vertexIndexBuffers;
	std::unordered_map<std::string, RenderAssetHandle> renderAssetPaths;
	std::vector<MeshInfo> meshes;
	std::vector<RenderAsset> renderAssets;

	struct PendingGeometryFree
	{
		GeometryAllocation allocation;
		uint64_t retireFrame;
	};
	std::vector<PendingGeometryFree> _pendingGeometryFrees;

	std::vector<int> _freeMeshSlots;
	std::vector<int> _freeRenderAssetSlots;

	VulkanDevice& vulkanDevice;
	VmaAllocator allocator = {};
};
