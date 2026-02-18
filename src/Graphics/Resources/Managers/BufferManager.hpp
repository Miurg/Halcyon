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
#include "ResourceHandles.hpp"
#include "../Components/BindlessTextureDSetComponent.hpp"
#include "../ResourceStructures.hpp"

class DescriptorManager;

// Manages GPU buffers — creates and tracks allocations via VMA.
class BufferManager
{
public:
	BufferManager(VulkanDevice& vulkanDevice, VmaAllocator allocator);
	~BufferManager();

	BufferHandle createBuffer(vk::MemoryPropertyFlags propertyBits, vk::DeviceSize sizeBuffer,
	                          uint_fast16_t numberBuffers, vk::Flags<vk::BufferUsageFlagBits> usageBuffer);

	template <typename T>
	void writeToBuffer(BufferHandle handle, uint32_t bufferIndex, uint32_t elementIndex, const T& value)
	{
		void* mapped = buffers[handle.id].bufferMapped[bufferIndex];
		memcpy(static_cast<T*>(mapped) + elementIndex, &value, sizeof(T));
	}

	std::vector<Buffer> buffers;

private:
	VulkanDevice& vulkanDevice;
	VmaAllocator allocator = {};

	void initGlobalBuffer(vk::MemoryPropertyFlags propertyBits, Buffer& bufferIn, vk::DeviceSize sizeBuffer,
	                      uint_fast16_t numberBuffers, vk::Flags<vk::BufferUsageFlagBits> usageBuffer);
};