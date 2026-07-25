#include "GraphicsCore/Resources/Factories/BufferFactory.hpp"
#include "GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"

BufferHandle BufferFactory::createStorageBuffer(BufferManager& bufferManager, DescriptorManager& descriptorManager,
                                                vk::MemoryPropertyFlags propertyBits, vk::DeviceSize sizeBuffer,
                                                uint_fast16_t numberBuffers,
                                                vk::Flags<vk::BufferUsageFlagBits> usageBuffer, DSetHandle dSet,
                                                uint32_t binding)
{
	BufferHandle handle = bufferManager.createBuffer(propertyBits, sizeBuffer, numberBuffers, usageBuffer);
	bindStorageBuffer(bufferManager, descriptorManager, handle, dSet, binding);
	return handle;
}

void BufferFactory::bindStorageBuffer(BufferManager& bufferManager, DescriptorManager& descriptorManager,
                                      BufferHandle handle, DSetHandle dSet, uint32_t binding)
{
	const uint32_t copies = descriptorManager.getSetCount(dSet);
	const bool sharedBuf = (bufferManager.bufferCopyCount(handle) == 1);

	for (uint32_t i = 0; i < copies; ++i)
		descriptorManager.update(dSet, binding, i, vk::DescriptorType::eStorageBuffer,
		                         bufferManager.getBuffer(handle, sharedBuf ? 0 : i));
}
