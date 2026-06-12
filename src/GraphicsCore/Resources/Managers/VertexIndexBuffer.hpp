#pragma once

#include <vector>
#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.h>
#include "Vertex.hpp"

class VertexIndexBuffer
{
public:
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	vk::Buffer vertexBuffer = nullptr;
	VmaAllocation vertexBufferAllocation = nullptr;
	vk::Buffer indexBuffer = nullptr;
	VmaAllocation indexBufferAllocation = nullptr;
};