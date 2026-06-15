#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/VertexIndexBuffer.hpp"
#include <vector>
#include <unordered_map>
#include "GraphicsCore/Resources/Components/MeshInfoComponent.hpp"
#include "GraphicsCore/VulkanConst.hpp"
#include <vk_mem_alloc.h>
#include "GraphicsCore/Resources/Managers/Texture.hpp"
#include "GraphicsCore/Resources/Managers/Buffer.hpp"
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include "GraphicsCore/VulkanDevice.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "GraphicsCore/Resources/Managers/MeshInfo.hpp"

// Stores loaded meshes and their GPU vertex/index buffers. Deduplicates by file path.
class HALCYON_API ModelManager
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