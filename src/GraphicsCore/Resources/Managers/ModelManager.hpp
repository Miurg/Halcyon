#pragma once

#include "VertexIndexBuffer.hpp"
#include <vector>
#include <unordered_map>
#include "GraphicsCore/Resources/Components/MeshInfoComponent.hpp"
#include "../../VulkanConst.hpp"
#include <vk_mem_alloc.h>
#include "Texture.hpp"
#include "Buffer.hpp"
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include "../../VulkanDevice.hpp"
#include "DescriptorManager.hpp"
#include "MeshInfo.hpp"

// Stores loaded meshes and their GPU vertex/index buffers. Deduplicates by file path.
class ModelManager
{
public:
	ModelManager(VulkanDevice& vulkanDevice, VmaAllocator allocator);
	~ModelManager();
	bool isMeshLoaded(const char path[MAX_PATH_LEN]);
	void createVertexBuffer(VertexIndexBuffer& vertexIndexBuffer);
	void createIndexBuffer(VertexIndexBuffer& vertexIndexBuffer);
	std::vector<VertexIndexBuffer> vertexIndexBuffers;
	std::unordered_map<std::string, int> meshPaths;
	std::vector<MeshInfo> meshes;

private:
	VulkanDevice& vulkanDevice;
	VmaAllocator allocator = {};
};