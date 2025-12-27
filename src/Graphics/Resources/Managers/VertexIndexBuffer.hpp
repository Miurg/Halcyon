#pragma once

#include "../../VulkanDevice.hpp"
#include <vector>
#include <vulkan/vulkan_raii.hpp>
#include <glm/glm.hpp>
#include <array>
#include <string>
#include <functional>
#include "Vertex.hpp"

class VertexIndexBuffer
{
public:
	const std::string MODEL_PATH = "assets/models/viking_room.obj";

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	vk::raii::Buffer vertexBuffer = nullptr;
	vk::raii::DeviceMemory vertexBufferMemory = nullptr;
	vk::raii::Buffer indexBuffer = nullptr;
	vk::raii::DeviceMemory indexBufferMemory = nullptr;

	void createVertexBuffer(VulkanDevice& vulkanDevice);
	void createIndexBuffer(VulkanDevice& vulkanDevice);
};