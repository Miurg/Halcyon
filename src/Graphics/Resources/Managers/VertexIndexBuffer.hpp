#pragma once

#include <vector>
#include <vulkan/vulkan_raii.hpp>
#include "Vertex.hpp"

class VertexIndexBuffer
{
public:
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	vk::raii::Buffer vertexBuffer = nullptr;
	vk::raii::DeviceMemory vertexBufferMemory = nullptr;
	vk::raii::Buffer indexBuffer = nullptr;
	vk::raii::DeviceMemory indexBufferMemory = nullptr;
};