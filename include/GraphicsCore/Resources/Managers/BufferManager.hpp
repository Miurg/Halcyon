#pragma once

#include <vector>
#include <unordered_map>
#include "GraphicsCore/Resources/Components/MeshInfoComponent.hpp"
#include "GraphicsCore/VulkanConst.hpp"
#include <vk_mem_alloc.h>
#include "GraphicsCore/Resources/Managers/Texture.hpp"
#include "GraphicsCore/Resources/Managers/Buffer.hpp"
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include "GraphicsCore/VulkanDevice.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include "GraphicsCore/Resources/ResourceStructures.hpp"

class DescriptorManager;
class TextureManager;
class PipelineManager;

// Manages GPU buffers — creates and tracks allocations via VMA.
class BufferManager
{
public:
	BufferManager(VulkanDevice& vulkanDevice, VmaAllocator allocator);
	~BufferManager();

	BufferHandle createBuffer(vk::MemoryPropertyFlags propertyBits, vk::DeviceSize sizeBuffer,
	                          uint_fast16_t numberBuffers, vk::Flags<vk::BufferUsageFlagBits> usageBuffer);
	void bakeSHForProbe(TextureHandle envCubemap, BufferHandle probeBuffer, int probeSlot,
	                    DescriptorManager& dManager, BindlessTextureDSetComponent& dSetComponent,
	                    DSetHandle globalDSet, PipelineManager& pManager, TextureManager& tManager);

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