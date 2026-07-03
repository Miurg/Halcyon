#pragma once

#include "HalcyonExport.hpp"
#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.h>
#include "GraphicsCore/Resources/Managers/RangeAllocator.hpp"

class HALCYON_API VertexIndexBuffer
{
public:
	vk::Buffer vertexBuffer = nullptr;
	VmaAllocation vertexBufferAllocation = nullptr;
	vk::Buffer indexBuffer = nullptr;
	VmaAllocation indexBufferAllocation = nullptr;

	RangeAllocator vertexAllocator;
	RangeAllocator indexAllocator;
};
