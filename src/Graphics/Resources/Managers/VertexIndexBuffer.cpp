#include "VertexIndexBuffer.hpp"
#include <stdexcept>
#include "../../VulkanUtils.hpp"
#include <unordered_map>
#include <iostream>

void VertexIndexBuffer::createVertexBuffer(VulkanDevice& vulkanDevice)
{
	vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	auto result = VulkanUtils::createBuffer(
	    bufferSize, vk::BufferUsageFlagBits::eVertexBuffer,
	    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vulkanDevice);

	vertexBuffer = std::move(result.first);
	vertexBufferMemory = std::move(result.second);

	void* data = vertexBufferMemory.mapMemory(0, bufferSize);
	memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
	vertexBufferMemory.unmapMemory();
}

void VertexIndexBuffer::createIndexBuffer(VulkanDevice& vulkanDevice)
{
	vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	auto result = VulkanUtils::createBuffer(
	    bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
	    vk::MemoryPropertyFlagBits::eHostVisible, vulkanDevice);

	indexBuffer = std::move(result.first);
	indexBufferMemory = std::move(result.second);

	void* data = indexBufferMemory.mapMemory(0, bufferSize);
	memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
	indexBufferMemory.unmapMemory();
}
