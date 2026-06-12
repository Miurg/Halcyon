#include "ModelManager.hpp"
#include <stdexcept>
#include "../../VulkanUtils.hpp"
#include <cstring>
#include "../Factories/GltfLoader.hpp"

ModelManager::ModelManager(VulkanDevice& vulkanDevice, VmaAllocator allocator)
    : vulkanDevice(vulkanDevice), allocator(allocator)
{
	vertexIndexBuffers.push_back(VertexIndexBuffer());
}

ModelManager::~ModelManager()
{
	for (auto& meshBuffer : vertexIndexBuffers)
	{
		if (meshBuffer.vertexBuffer)
		{
			vmaDestroyBuffer(allocator, meshBuffer.vertexBuffer, meshBuffer.vertexBufferAllocation);
		}
		if (meshBuffer.indexBuffer)
		{
			vmaDestroyBuffer(allocator, meshBuffer.indexBuffer, meshBuffer.indexBufferAllocation);
		}
	}
}

bool ModelManager::isMeshLoaded(const char path[MAX_PATH_LEN])
{
	std::string pathStr(path);
	return meshPaths.find(pathStr) != meshPaths.end();
}

void ModelManager::createVertexBuffer(VertexIndexBuffer& vertexIndexBuffer)
{
	if (vertexIndexBuffer.vertexBuffer)
	{
		vmaDestroyBuffer(allocator, vertexIndexBuffer.vertexBuffer, vertexIndexBuffer.vertexBufferAllocation);
		vertexIndexBuffer.vertexBuffer = nullptr;
		vertexIndexBuffer.vertexBufferAllocation = nullptr;
	}

	vk::DeviceSize bufferSize = sizeof(vertexIndexBuffer.vertices[0]) * vertexIndexBuffer.vertices.size();
	if (bufferSize == 0) return;

	vk::BufferCreateInfo bufferInfo;
	bufferInfo.size = bufferSize;
	bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

	VkBuffer buffer;
	VmaAllocation allocation;
	VkBufferCreateInfo bufferInfoC = (VkBufferCreateInfo)bufferInfo;

	if (vmaCreateBuffer(allocator, &bufferInfoC, &allocInfo, &buffer, &allocation, nullptr) !=
	    VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create device-local vertex buffer!");
	}

	vertexIndexBuffer.vertexBuffer = vk::Buffer(buffer);
	vertexIndexBuffer.vertexBufferAllocation = allocation;
	
	// Staging buffer copy
	auto staging = VulkanUtils::createStagingBuffer(vertexIndexBuffer.vertices.data(), bufferSize, allocator);

	auto cmd = VulkanUtils::beginSingleTimeCommands(vulkanDevice);
	vk::BufferCopy copyRegion{0, 0, bufferSize};
	cmd.copyBuffer(staging.buffer, vertexIndexBuffer.vertexBuffer, copyRegion);
	VulkanUtils::endSingleTimeCommands(cmd, vulkanDevice);

	VulkanUtils::destroyStagingBuffer(staging, allocator);
}

void ModelManager::createIndexBuffer(VertexIndexBuffer& vertexIndexBuffer)
{
	if (vertexIndexBuffer.indexBuffer)
	{
		vmaDestroyBuffer(allocator, vertexIndexBuffer.indexBuffer, vertexIndexBuffer.indexBufferAllocation);
		vertexIndexBuffer.indexBuffer = nullptr;
		vertexIndexBuffer.indexBufferAllocation = nullptr;
	}

	vk::DeviceSize bufferSize = sizeof(vertexIndexBuffer.indices[0]) * vertexIndexBuffer.indices.size();
	if (bufferSize == 0) return;

	vk::BufferCreateInfo bufferInfo;
	bufferInfo.size = bufferSize;
	bufferInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

	VkBuffer buffer;
	VmaAllocation allocation;

	VkBufferCreateInfo bufferInfoC = (VkBufferCreateInfo)bufferInfo;
	
	if (vmaCreateBuffer(allocator, &bufferInfoC, &allocInfo, &buffer, &allocation, nullptr) !=
	    VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create index buffer!");
	}

	vertexIndexBuffer.indexBuffer = vk::Buffer(buffer);
	vertexIndexBuffer.indexBufferAllocation = allocation;
	
	// Staging buffer copy
	auto staging = VulkanUtils::createStagingBuffer(vertexIndexBuffer.indices.data(), bufferSize, allocator);

	auto cmd = VulkanUtils::beginSingleTimeCommands(vulkanDevice);
	vk::BufferCopy copyRegion{0, 0, bufferSize};
	cmd.copyBuffer(staging.buffer, vertexIndexBuffer.indexBuffer, copyRegion);
	VulkanUtils::endSingleTimeCommands(cmd, vulkanDevice);

	VulkanUtils::destroyStagingBuffer(staging, allocator);
}
