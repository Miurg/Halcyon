#pragma once

#include "HalcyonExport.hpp"
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
class HALCYON_API BufferManager
{
public:
	BufferManager(VulkanDevice& vulkanDevice, VmaAllocator allocator);
	~BufferManager();

	BufferHandle createBuffer(vk::MemoryPropertyFlags propertyBits, vk::DeviceSize sizeBuffer,
	                          uint_fast16_t numberBuffers, vk::Flags<vk::BufferUsageFlagBits> usageBuffer);
	void bakeSHForProbe(TextureHandle envCubemap, BufferHandle probeBuffer, int probeSlot,
	                    DescriptorManager& descriptorManager, BindlessTextureDSetComponent& dSetComponent,
	                    DSetHandle globalDSet, PipelineManager& pipelineManager, TextureManager& textureManager);
	// Records the sh_projection dispatch only; GICaptureCubemap descriptor must already point at the input.
	void recordSHProjection(vk::raii::CommandBuffer& cmd, int cubemapResolution, int probeSlot,
	                        DescriptorManager& descriptorManager, BindlessTextureDSetComponent& dSetComponent,
	                        DSetHandle globalDSet, PipelineManager& pipelineManager);

	template <typename T>
	void writeToBuffer(BufferHandle handle, uint32_t bufferIndex, uint32_t elementIndex, const T& value)
	{
		void* mapped = buffers[handle.id].bufferMapped[bufferIndex];
		memcpy(static_cast<T*>(mapped) + elementIndex, &value, sizeof(T));
	}

	vk::Buffer getBuffer(BufferHandle handle, uint32_t index = 0) const;
	template <typename T>
	T* getMapped(BufferHandle handle, uint32_t index = 0) const
	{
		return static_cast<T*>(buffers[handle.id].bufferMapped[index]);
	}

	std::vector<Buffer> buffers;

private:
	VulkanDevice& vulkanDevice;
	VmaAllocator allocator = {};

	void initGlobalBuffer(vk::MemoryPropertyFlags propertyBits, Buffer& bufferIn, vk::DeviceSize sizeBuffer,
	                      uint_fast16_t numberBuffers, vk::Flags<vk::BufferUsageFlagBits> usageBuffer);
};