#pragma once

#include "VertexIndexBuffer.hpp"
#include <vector>
#include <unordered_map>
#include "../Components/MeshInfoComponent.hpp"
#include "../../VulkanConst.hpp"
#include <vk_mem_alloc.h>
#include "Texture.hpp"
#include "Buffer.hpp"
#include "../Components/BindlessTextureDSetComponent.hpp"
#include "../../VulkanDevice.hpp"
#include "PrimitivesInfo.hpp"
#include "MeshInfo.hpp"
#include "DescriptorManager.hpp"
#include "../Components/BindlessTextureDSetComponent.hpp"

class DescriptorManager;

class BufferManager
{
public:
	BufferManager(VulkanDevice& vulkanDevice, VmaAllocator allocator);
	~BufferManager();

	bool isMeshLoaded(const char path[MAX_PATH_LEN]);
	int createBuffer(vk::MemoryPropertyFlags propertyBits, vk::DeviceSize sizeBuffer, uint_fast16_t numberBuffers,
	                 uint_fast16_t numberBinding, vk::DescriptorSetLayout layout);

	void createVertexBuffer(VertexIndexBuffer& vertexIndexBuffer);
	void createIndexBuffer(VertexIndexBuffer& vertexIndexBuffer);
	std::vector<VertexIndexBuffer> vertexIndexBuffers;
	std::unordered_map<std::string, int> meshPaths;
	std::vector<MeshInfo> meshes;
	std::vector<Buffer> buffers;
	

private:
	VulkanDevice& vulkanDevice;
	VmaAllocator allocator = {};

	void initGlobalBuffer(vk::MemoryPropertyFlags propertyBits, Buffer& bufferIn, vk::DeviceSize sizeBuffer,
	                      uint_fast16_t numberBuffers);
	//int createMeshInternal(const char path[MAX_PATH_LEN], BindlessTextureDSetComponent& dSetComponent,
	//                       DescriptorManager& dManager);
};

struct CameraStucture
{
	alignas(16) glm::mat4 cameraSpaceMatrix;
};

struct SunStructure
{
	alignas(16) glm::mat4 lightSpaceMatrix;
	glm::vec4 direction; // xyz: direction, w: padding
	glm::vec4 color;     // rgb: color, w: intensity of light
	glm::vec4 ambient;   // rgb: ambient color, w: intensity of ambient
};

struct PrimitiveSctructure // (16 + 4 + 4 + padding = 32 bytes)
{
	alignas(16) glm::vec4 baseColor; // 16 bytes
	uint32_t transformIndex; // 4 bytes
	uint32_t textureIndex;   // 4 bytes
	uint32_t _padding[2];
};

struct TransformStructure
{
	alignas(16) glm::mat4 model;
};