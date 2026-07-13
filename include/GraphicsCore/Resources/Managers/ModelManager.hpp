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

struct HALCYON_API GeometryAllocation
{
	uint32_t vertexBase = 0;
	uint32_t vertexCount = 0;
	uint32_t indexBase = 0;
	uint32_t indexCount = 0;
	int bufferIndex = 0;
};

struct HALCYON_API Model
{
	GeometryAllocation allocation;
	std::vector<int> meshes;
	std::vector<int> textures;
	std::vector<int> materials;
	int refCount = 0;
};

// Stores loaded meshes and their GPU vertex/index buffers. Deduplicates by file path.
class HALCYON_API ModelManager
{
public:
	ModelManager(VulkanDevice& vulkanDevice, VmaAllocator allocator);
	~ModelManager();
	bool isModelLoaded(const char path[MAX_PATH_LEN]);

	std::optional<GeometryAllocation> allocateGeometry(int bufferIndex, uint32_t vertexCount, uint32_t indexCount);
	void uploadVertices(int bufferIndex, uint32_t vertexBase, const Vertex* data, uint32_t count);
	void uploadIndices(int bufferIndex, uint32_t indexBase, const uint32_t* data, uint32_t count);
	void freeGeometry(const GeometryAllocation& allocation, uint64_t frameNumber);
	void collectGeometryFrees(uint64_t frameNumber);
	void defragment(VertexIndexBuffer& buffer);

	int allocateMeshSlot();
	int allocateModelSlot();
	void freeMeshSlot(int slot);
	void freeModelSlot(int slot);
	size_t freeMeshSlotCount() const;
	size_t freeModelSlotCount() const;
	size_t pendingGeometryFreeCount() const;

	VertexIndexBuffer& getVertexIndexBuffer(int index);
	MeshInfo& getMesh(int slot);
	Model& getModel(int slot);

	std::vector<VertexIndexBuffer> vertexIndexBuffers;
	std::unordered_map<std::string, int> modelPaths;
	std::vector<MeshInfo> meshes;
	std::vector<Model> models;

private:
	struct PendingGeometryFree
	{
		GeometryAllocation allocation;
		uint64_t retireFrame;
	};
	std::vector<PendingGeometryFree> _pendingGeometryFrees;

	std::vector<int> _freeMeshSlots;
	std::vector<int> _freeModelSlots;

	VulkanDevice& vulkanDevice;
	VmaAllocator allocator = {};
};
