#include "BufferManager.hpp"
#include <stdexcept>
#include "../Factories/LoadFileFactory.hpp"
#include "../../VulkanUtils.hpp"

MeshInfoComponent BufferManager::createMesh(const char path[MAX_PATH_LEN])
{
	if (isMeshLoaded(path))
	{
		return meshPaths[std::string(path)];
	}
	for (int i = 0; i < meshes.size(); i++)
	{
		if (sizeof(meshes[i].vertices) < MAX_SIZE_OF_VERTEX_INDEX_BUFFER)
		{
			MeshInfoComponent info = LoadFileFactory::addMeshFromFile(path, meshes[i]);
			info.bufferIndex = static_cast<uint32_t>(i);
			createVertexBuffer(vulkanDevice, meshes[i]);
			createIndexBuffer(vulkanDevice, meshes[i]);
			meshPaths[std::string(path)] = info;
			return info;
		}
	}

	meshes.push_back(VertexIndexBuffer());
	MeshInfoComponent info = LoadFileFactory::addMeshFromFile(path, meshes.back());
	info.bufferIndex = static_cast<uint32_t>(meshes.size() - 1);
	createVertexBuffer(vulkanDevice, meshes.back());
	createIndexBuffer(vulkanDevice, meshes.back());
	meshPaths[std::string(path)] = info;
	return info;
}

bool BufferManager::isMeshLoaded(const char path[MAX_PATH_LEN])
{
	std::string pathStr(path);
	return meshPaths.find(pathStr) != meshPaths.end();
}

void BufferManager::createVertexBuffer(VulkanDevice& vulkanDevice, VertexIndexBuffer& vertexIndexBuffer)
{
	vk::DeviceSize bufferSize = sizeof(vertexIndexBuffer.vertices[0]) * vertexIndexBuffer.vertices.size();

	auto result = VulkanUtils::createBuffer(
	    bufferSize, vk::BufferUsageFlagBits::eVertexBuffer,
	    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vulkanDevice);

	vertexIndexBuffer.vertexBuffer = std::move(result.first);
	vertexIndexBuffer.vertexBufferMemory = std::move(result.second);

	void* data = vertexIndexBuffer.vertexBufferMemory.mapMemory(0, bufferSize);
	memcpy(data, vertexIndexBuffer.vertices.data(), static_cast<size_t>(bufferSize));
	vertexIndexBuffer.vertexBufferMemory.unmapMemory();
}

void BufferManager::createIndexBuffer(VulkanDevice& vulkanDevice, VertexIndexBuffer& vertexIndexBuffer)
{
	vk::DeviceSize bufferSize = sizeof(vertexIndexBuffer.indices[0]) * vertexIndexBuffer.indices.size();

	auto result = VulkanUtils::createBuffer(
	    bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
	    vk::MemoryPropertyFlagBits::eHostVisible, vulkanDevice);

	vertexIndexBuffer.indexBuffer = std::move(result.first);
	vertexIndexBuffer.indexBufferMemory = std::move(result.second);

	void* data = vertexIndexBuffer.indexBufferMemory.mapMemory(0, bufferSize);
	memcpy(data, vertexIndexBuffer.indices.data(), static_cast<size_t>(bufferSize));
	vertexIndexBuffer.indexBufferMemory.unmapMemory();
}
