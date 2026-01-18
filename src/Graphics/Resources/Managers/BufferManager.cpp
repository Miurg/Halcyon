#include "BufferManager.hpp"
#include <stdexcept>
#include "../Factories/LoadFileFactory.hpp"
#include "DescriptorManager.hpp"

BufferManager::BufferManager(VulkanDevice& vulkanDevice) : vulkanDevice(vulkanDevice) 
{
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = *vulkanDevice.physicalDevice;
	allocatorInfo.device = *vulkanDevice.device;
	allocatorInfo.instance = *vulkanDevice.instance;
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_4;
	VmaVulkanFunctions vulkanFunctions = {};
	vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

	allocatorInfo.pVulkanFunctions = &vulkanFunctions;
	VkResult result = vmaCreateAllocator(&allocatorInfo, &this->allocator);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create VMA allocator!");
	}
}

BufferManager::~BufferManager() 
{
	for (auto& texture : textures)
	{
		if (texture.textureSampler)
		{
			(*vulkanDevice.device).destroySampler(texture.textureSampler);
		}

		if (texture.textureImageView)
		{
			(*vulkanDevice.device).destroyImageView(texture.textureImageView);
		}

		if (texture.textureImage)
		{
			vmaDestroyImage(allocator, texture.textureImage, texture.textureImageAllocation);
		}
	}
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

int BufferManager::createBuffer(vk::MemoryPropertyFlags propertyBits,
                                vk::DeviceSize sizeBuffer,
                                uint_fast16_t numberBuffers, uint_fast16_t numberBinding,
                                vk::DescriptorSetLayout layout)
{
	buffers.push_back(Buffer());
	BufferManager::initGlobalBuffer(propertyBits, buffers.back(), sizeBuffer, numberBuffers);
	return buffers.size() - 1;
}

void BufferManager::initGlobalBuffer(vk::MemoryPropertyFlags propertyBits, Buffer& bufferIn, vk::DeviceSize sizeBuffer,
                                     uint_fast16_t numberBuffers)
{
	vk::BufferCreateInfo bufferInfo;
	bufferInfo.size = sizeBuffer;
	bufferInfo.usage = vk::BufferUsageFlagBits::eStorageBuffer;
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

