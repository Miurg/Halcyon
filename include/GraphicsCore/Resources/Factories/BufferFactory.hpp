#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"
#include <vulkan/vulkan.hpp>
#include <cstdint>

class BufferManager;
class DescriptorManager;

class HALCYON_API BufferFactory
{
public:
	static BufferHandle createStorageBuffer(BufferManager& bufferManager, DescriptorManager& descriptorManager,
	                                        vk::MemoryPropertyFlags propertyBits, vk::DeviceSize sizeBuffer,
	                                        uint_fast16_t numberBuffers,
	                                        vk::Flags<vk::BufferUsageFlagBits> usageBuffer, DSetHandle dSet,
	                                        uint32_t binding);

	static void bindStorageBuffer(BufferManager& bufferManager, DescriptorManager& descriptorManager,
	                              BufferHandle handle, DSetHandle dSet, uint32_t binding);
};
