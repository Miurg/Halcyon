#include "BufferManager.hpp"
#include "DescriptorManager.hpp"
#include "TextureManager.hpp"
#include "../../Managers/PipelineManager.hpp"
#include "../../VulkanUtils.hpp"
#include <stdexcept>

BufferManager::BufferManager(VulkanDevice& vulkanDevice, VmaAllocator allocator)
    : vulkanDevice(vulkanDevice), allocator(allocator)
{
}

BufferManager::~BufferManager()
{
	for (auto& buffer : buffers)
	{
		for (size_t i = 0; i < buffer.buffer.size(); ++i)
		{
			if (buffer.buffer[i])
			{
				vmaDestroyBuffer(allocator, buffer.buffer[i], buffer.bufferAllocation[i]);
			}
		}
	}
}

BufferHandle BufferManager::createBuffer(vk::MemoryPropertyFlags propertyBits, vk::DeviceSize sizeBuffer,
                                         uint_fast16_t numberBuffers, vk::Flags<vk::BufferUsageFlagBits> usageBuffer)
{
	buffers.push_back(Buffer());
	BufferManager::initGlobalBuffer(propertyBits, buffers.back(), sizeBuffer, numberBuffers, usageBuffer);
	return BufferHandle{static_cast<int>(buffers.size() - 1)};
}

BufferHandle BufferManager::generateSHCoefficients(TextureHandle envCubemap, DescriptorManager& dManager,
                                                    BindlessTextureDSetComponent& dSetComponent,
                                                    PipelineManager& pManager, TextureManager& tManager)
{
	constexpr vk::DeviceSize bufferSize = sizeof(float) * 4 * 9; // SHCoefficients: float4[9]

	BufferHandle handle =
	    createBuffer(vk::MemoryPropertyFlagBits::eDeviceLocal, bufferSize, 1,
	                 vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);

	dManager.updateSHBufferDescriptor(dSetComponent, buffers[handle.id].buffer[0], bufferSize);

	// Cubemap was generated at 1024x1024
	constexpr int cubemapResolution = 1024; //TODO: Get rid of hardcoded resolution. Same in cubemap compute.

	auto cmd = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["sh_projection"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["sh_projection"].layout, 0,
	                       dManager.descriptorSets[dSetComponent.bindlessTextureSet.id][0], nullptr);

	cmd.pushConstants<int>(*pManager.pipelines["sh_projection"].layout, vk::ShaderStageFlagBits::eCompute, 0,
	                       cubemapResolution);

	cmd.dispatch(1, 1, 1);

	vk::MemoryBarrier2 barrier;
	barrier.srcStageMask  = vk::PipelineStageFlagBits2::eComputeShader;
	barrier.srcAccessMask = vk::AccessFlagBits2::eShaderWrite;
	barrier.dstStageMask  = vk::PipelineStageFlagBits2::eFragmentShader;
	barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;

	vk::DependencyInfo depInfo;
	depInfo.memoryBarrierCount = 1;
	depInfo.pMemoryBarriers    = &barrier;
	cmd.pipelineBarrier2(depInfo);

	VulkanUtils::endSingleTimeCommands(cmd, vulkanDevice);

	return handle;
}

void BufferManager::initGlobalBuffer(vk::MemoryPropertyFlags propertyBits, Buffer& bufferIn, vk::DeviceSize sizeBuffer,
                                     uint_fast16_t numberBuffers, vk::Flags<vk::BufferUsageFlagBits> usageBuffer)
{
	vk::BufferCreateInfo bufferInfo;
	bufferInfo.size = sizeBuffer;
	bufferInfo.usage = usageBuffer;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

	if (propertyBits & vk::MemoryPropertyFlagBits::eHostVisible)
	{
		allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		allocInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}
	if (propertyBits & vk::MemoryPropertyFlagBits::eDeviceLocal)
	{
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	}

	for (size_t i = 0; i < numberBuffers; i++)
	{
		VkBuffer bufferC;
		VmaAllocation allocation;
		VmaAllocationInfo resultInfo;

		VkBufferCreateInfo bufferInfoC = (VkBufferCreateInfo)bufferInfo;

		vmaCreateBuffer(allocator, &bufferInfoC, &allocInfo, &bufferC, &allocation, &resultInfo);

		bufferIn.buffer.push_back(vk::Buffer(bufferC));
		bufferIn.bufferAllocation.push_back(allocation);

		if (propertyBits & vk::MemoryPropertyFlagBits::eHostVisible)
		{
			bufferIn.bufferMapped.push_back(resultInfo.pMappedData);
		}
	}
}

