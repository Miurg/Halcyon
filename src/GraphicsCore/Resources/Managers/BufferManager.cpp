#include "GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "GraphicsCore/Resources/Managers/Bindings.hpp"
#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Managers/PipelineManager.hpp"
#include "GraphicsCore/VulkanUtils.hpp"
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

void BufferManager::bakeSHForProbe(TextureHandle envCubemap, BufferHandle probeBuffer, int probeSlot,
                                   DescriptorManager& descriptorManager, BindlessTextureDSetComponent& dSetComponent,
                                   DSetHandle globalDSet, PipelineManager& pipelineManager, TextureManager& textureManager)
{
	Texture& envTex = textureManager.textures[envCubemap.id];
	descriptorManager.update(dSetComponent.bindlessTextureSet, Bindings::Textures::GICaptureCubemap, 0,
	                vk::DescriptorType::eCombinedImageSampler, envTex.textureImageView,
	                textureManager.getSampler(envTex.samplerHandle));

	auto cmd = VulkanUtils::beginSingleTimeCommands(vulkanDevice);
	recordSHProjection(cmd, static_cast<int>(envTex.width), probeSlot, descriptorManager, dSetComponent, globalDSet, pipelineManager);
	VulkanUtils::endSingleTimeCommands(cmd, vulkanDevice);
}

void BufferManager::recordSHProjection(vk::raii::CommandBuffer& cmd, int cubemapResolution, int probeSlot,
                                       DescriptorManager& descriptorManager, BindlessTextureDSetComponent& dSetComponent,
                                       DSetHandle globalDSet, PipelineManager& pipelineManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pipelineManager.pipelines["sh_projection"].pipeline);

	// sh_projection: set 0 = globalSet (SHProbeEntry[] output), set 1 = textureSet (cubemap input)
	std::array<vk::DescriptorSet, 2> sets = {
	    descriptorManager.getSet(globalDSet),
	    descriptorManager.getSet(dSetComponent.bindlessTextureSet),
	};
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pipelineManager.pipelines["sh_projection"].layout,
	                       0, sets, nullptr);

	struct PushData { int cubemapResolution; int probeSlot; };
	PushData pushData = { cubemapResolution, probeSlot };
	cmd.pushConstants(*pipelineManager.pipelines["sh_projection"].layout, vk::ShaderStageFlagBits::eCompute,
	                  0, vk::ArrayProxy<const PushData>(1, &pushData));

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

vk::Buffer BufferManager::getBuffer(BufferHandle handle, uint32_t index) const
{
	return buffers[handle.id].buffer[index];
}