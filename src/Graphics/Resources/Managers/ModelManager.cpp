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

	vk::BufferCreateInfo bufferInfo;
	bufferInfo.size = bufferSize;
	bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	allocInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VkBuffer buffer;
	VmaAllocation allocation;
	VmaAllocationInfo allocResultInfo;

	VkBufferCreateInfo bufferInfoC = (VkBufferCreateInfo)bufferInfo;
	vmaCreateBuffer(allocator, &bufferInfoC, &allocInfo, &buffer, &allocation, &allocResultInfo);

	vertexIndexBuffer.vertexBuffer = vk::Buffer(buffer);
	vertexIndexBuffer.vertexBufferAllocation = allocation;

	void* data = allocResultInfo.pMappedData;
	memcpy(data, vertexIndexBuffer.vertices.data(), static_cast<size_t>(bufferSize));
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

	vk::BufferCreateInfo bufferInfo;
	bufferInfo.size = bufferSize;
	bufferInfo.usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	allocInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VkBuffer buffer;
	VmaAllocation allocation;
	VmaAllocationInfo allocResultInfo;

	VkBufferCreateInfo bufferInfoC = (VkBufferCreateInfo)bufferInfo;
	vmaCreateBuffer(allocator, &bufferInfoC, &allocInfo, &buffer, &allocation, &allocResultInfo);

	vertexIndexBuffer.indexBuffer = vk::Buffer(buffer);
	vertexIndexBuffer.indexBufferAllocation = allocation;

	void* data = allocResultInfo.pMappedData;
	memcpy(data, vertexIndexBuffer.indices.data(), static_cast<size_t>(bufferSize));
}
