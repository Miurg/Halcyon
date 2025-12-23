#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "VulkanConst.hpp"
#include "VulkanDevice.hpp"
#include "VulkanUtils.hpp"
#include "Texture.hpp" // Добавлено: декларация Texture

struct GameObject
{
	glm::vec3 position = {0.0f, 0.0f, 0.0f};
	glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
	glm::vec3 scale = {1.0f, 1.0f, 1.0f};

	std::vector<vk::raii::Buffer> uniformBuffers;
	std::vector<vk::raii::DeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;

	std::vector<vk::raii::DescriptorSet> descriptorSets;

	Texture* texture = new Texture;

	glm::mat4 getModelMatrix() const
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, position);
		model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, scale);
		return model;
	}

	static void initUniformBuffers(GameObject& gameObject, VulkanDevice& vulkanDevice)
	{
		gameObject.uniformBuffers.clear();
		gameObject.uniformBuffersMemory.clear();
		gameObject.uniformBuffersMapped.clear();

		// Create uniform buffers for each frame in flight
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
			auto [buffer, bufferMem] = VulkanUtils::createBuffer(
			    bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
			    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vulkanDevice);
			gameObject.uniformBuffers.emplace_back(std::move(buffer));
			gameObject.uniformBuffersMemory.emplace_back(std::move(bufferMem));
			gameObject.uniformBuffersMapped.emplace_back(gameObject.uniformBuffersMemory[i].mapMemory(0, bufferSize));
		}
	};
};