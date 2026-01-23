#include "BufferManager.hpp"
#include <stdexcept>
#include "../Factories/LoadFileFactory.hpp"
#include "../../VulkanUtils.hpp"

int BufferManager::createMesh(const char path[MAX_PATH_LEN])
{
	if (isMeshLoaded(path))
	{
		return meshPaths[std::string(path)];
	}
	for (int i = 0; i < meshBuffers.size(); i++)
	{
		if (sizeof(meshBuffers[i].vertices) < MAX_SIZE_OF_VERTEX_INDEX_BUFFER)
		{
			MeshInfo info = LoadFileFactory::addMeshFromFile(path, meshBuffers[i]);
			info.bufferIndex = static_cast<uint32_t>(i);
			createVertexBuffer(vulkanDevice, meshBuffers[i]);
			createIndexBuffer(vulkanDevice, meshBuffers[i]);
			meshes.push_back(info);
			meshPaths[std::string(path)] = meshes.size()-1;
			return meshes.size() - 1;
		}
	}

	meshBuffers.push_back(VertexIndexBuffer());
	MeshInfo info = LoadFileFactory::addMeshFromFile(path, meshBuffers.back());
	info.bufferIndex = static_cast<uint32_t>(meshBuffers.size() - 1);
	createVertexBuffer(vulkanDevice, meshBuffers.back());
	createIndexBuffer(vulkanDevice, meshBuffers.back());
	meshes.push_back(info);
	meshPaths[std::string(path)] = meshes.size() - 1;
	return meshes.size() - 1;
}

bool BufferManager::isMeshLoaded(const char path[MAX_PATH_LEN])
{
	std::string pathStr(path);
	return meshPaths.find(pathStr) != meshPaths.end();
}

void BufferManager::createVertexBuffer(VulkanDevice& vulkanDevice, VertexIndexBuffer& vertexIndexBuffer)
{
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

void BufferManager::createIndexBuffer(VulkanDevice& vulkanDevice, VertexIndexBuffer& vertexIndexBuffer)
{
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
