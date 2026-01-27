#include "BufferManager.hpp"
#include <stdexcept>
#include "../../VulkanUtils.hpp"
#include <cstring>
#include "../Factories/GltfLoader.hpp"

int BufferManager::createMeshInternal(const char path[MAX_PATH_LEN], BindlessTextureDSetComponent& dSetComponent,
                                           DescriptorManager& dManager)
{
	auto infoVector = GltfLoader::loadMeshFromFile(path, vertexIndexBuffers.back());
	if (infoVector.empty())
	{
		throw std::runtime_error("Failed to load mesh from file: " + std::string(path));
	}
	MeshInfo meshInfo;
	for (const auto& loadedPrimitive : infoVector)
	{
		PrimitivesInfo info = loadedPrimitive.info;

		int texWidth = loadedPrimitive.texture.get()->width;
		int texHeight = loadedPrimitive.texture.get()->height;
		auto texturePtr = loadedPrimitive.texture; // shared_ptr<TextureData>
		if (texturePtr && !texturePtr->pixels.empty())
		{
			size_t expectedBytes = static_cast<size_t>(texWidth) * static_cast<size_t>(texHeight) * 4;
			if (texturePtr->pixels.size() < expectedBytes)
			{
				throw std::runtime_error("Texture pixel data size mismatch for " + std::string(path));
			}
			info.textureIndex =
			    generateTextureData(path, texWidth, texHeight, texturePtr->pixels.data(), dSetComponent, dManager);
		}
		createVertexBuffer(vulkanDevice, vertexIndexBuffers.back());
		createIndexBuffer(vulkanDevice, vertexIndexBuffers.back());
		meshInfo.primitives.push_back(info);
		
	}
	meshInfo.vertexIndexBufferID = static_cast<uint32_t>(vertexIndexBuffers.size() - 1);
	strcpy(meshInfo.path, path);
	meshes.push_back(meshInfo);
	meshPaths[std::string(path)] = meshes.size() - 1;
	return meshes.size() - 1;
}

int BufferManager::createMesh(const char path[MAX_PATH_LEN], BindlessTextureDSetComponent& dSetComponent,
                              DescriptorManager& dManager)
{
	if (isMeshLoaded(path))
	{
		return meshPaths[std::string(path)];
	}
	for (int i = 0; i < vertexIndexBuffers.size(); i++)
	{
		if (sizeof(vertexIndexBuffers[i].vertices) < MAX_SIZE_OF_VERTEX_INDEX_BUFFER)
		{
			return createMeshInternal(path, dSetComponent, dManager);	
		}
	}

	vertexIndexBuffers.push_back(VertexIndexBuffer());

	return createMeshInternal(path, dSetComponent, dManager);
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
