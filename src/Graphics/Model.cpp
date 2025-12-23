#include "Model.hpp"
#include <stdexcept>
#include "VulkanUtils.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>
#include <iostream>

Model::Model(VulkanDevice& vulkanDevice) : vulkanDevice(vulkanDevice)
{
	Model::loadModel();
	Model::createVertexBuffer();
	Model::createIndexBuffer();
}

Model::~Model() {}

void Model::loadModel()
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str()))
	{
		throw std::runtime_error(warn + err);
	}
	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};

			vertex.pos = {attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1],
			              attrib.vertices[3 * index.vertex_index + 2]};

			vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
			                   1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
			vertex.color = {1.0f, 1.0f, 1.0f};

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}
	std::cout << "Model loaded: " << vertices.size() << " vertices, " << indices.size() << " indices." << std::endl;
}

void Model::createVertexBuffer()
{
	vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	// Create a staging buffer in host visible memory
	auto [stagingBuffer, stagingBufferMemory] = VulkanUtils::createBuffer(
	    bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
	    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vulkanDevice);

	void* dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
	memcpy(dataStaging, vertices.data(), static_cast<size_t>(bufferSize));
	stagingBufferMemory.unmapMemory();

	// Create the vertex buffer with device local memory
	auto result = VulkanUtils::createBuffer(
	    bufferSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
	    vk::MemoryPropertyFlagBits::eDeviceLocal, vulkanDevice);
	vertexBuffer = std::move(result.first);
	vertexBufferMemory = std::move(result.second);

	// Copy data from the staging buffer to the vertex buffer
	VulkanUtils::copyBuffer(stagingBuffer, vertexBuffer, bufferSize, vulkanDevice);
}

void Model::createIndexBuffer()
{
	vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	auto [stagingBuffer, stagingBufferMemory] = VulkanUtils::createBuffer(
	    bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
	    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vulkanDevice);

	void* data = stagingBufferMemory.mapMemory(0, bufferSize);
	memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
	stagingBufferMemory.unmapMemory();

	auto result = VulkanUtils::createBuffer(
	    bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
	    vk::MemoryPropertyFlagBits::eDeviceLocal, vulkanDevice);

	indexBuffer = std::move(result.first);
	indexBufferMemory = std::move(result.second);

	VulkanUtils::copyBuffer(stagingBuffer, indexBuffer, bufferSize, vulkanDevice);
}
