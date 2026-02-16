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
#include "ResourceHandles.hpp"
#include "../Components/BindlessTextureDSetComponent.hpp"

class DescriptorManager;

class BufferManager
{
public:
	BufferManager(VulkanDevice& vulkanDevice, VmaAllocator allocator);
	~BufferManager();

	BufferHandle createBuffer(vk::MemoryPropertyFlags propertyBits, vk::DeviceSize sizeBuffer,
	                          uint_fast16_t numberBuffers, vk::Flags<vk::BufferUsageFlagBits> usageBuffer);

	std::vector<Buffer> buffers;

private:
	VulkanDevice& vulkanDevice;
	VmaAllocator allocator = {};

	void initGlobalBuffer(vk::MemoryPropertyFlags propertyBits, Buffer& bufferIn, vk::DeviceSize sizeBuffer,
	                      uint_fast16_t numberBuffers, vk::Flags<vk::BufferUsageFlagBits> usageBuffer);
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

struct PrimitiveSctructure // (16 + 16 + 16 + 4 + 4 + 8 = 64 bytes)
{
	alignas(16) glm::vec4 baseColor; // rgb: base color, w: alpha
	alignas(16) glm::vec3 AABBMin;   // xyz: min
	float padding0;                  // w: padding
	alignas(16) glm::vec3 AABBMax;   // xyz: max
	float padding1;                  // w: padding
	uint32_t transformIndex;         // index to the transform of the primitive
	uint32_t textureIndex;           // index to the texture of the primitive
	uint32_t drawCommandIndex;       // index to the indirect draw command of the primitive
	uint32_t _padding;               // 4 bytes (to make the total size a multiple of 16 bytes)
};

struct TransformStructure
{
	alignas(16) glm::mat4 model;
};

struct IndirectDrawStructure
{
	uint32_t indexCount;
	uint32_t instanceCount;
	uint32_t firstIndex;
	int vertexOffset;
	uint32_t firstInstance;
};