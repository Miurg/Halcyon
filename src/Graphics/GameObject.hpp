#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "VulkanConst.hpp"
#include "VulkanDevice.hpp"
#include "VulkanUtils.hpp"
#include "Resources/Managers/Texture.hpp"
#include <iostream>
#include "Resources/Components/MeshInfoComponent.hpp"


struct GameObject
{
	std::vector<vk::raii::Buffer> modelBuffers;
	std::vector<vk::raii::DeviceMemory> modelBuffersMemory;
	std::vector<void*> modelBuffersMapped;

	std::vector<vk::raii::DescriptorSet> modelDescriptorSets;
	vk::raii::DescriptorSet textureDescriptorSet = nullptr;

	static void initUniformBuffers(GameObject& gameObject, VulkanDevice& vulkanDevice)
	{
		gameObject.modelBuffers.clear();
		gameObject.modelBuffersMemory.clear();
		gameObject.modelBuffersMapped.clear();

		// Create uniform buffers for each frame in flight
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
			auto [buffer, bufferMem] = VulkanUtils::createBuffer(
			    bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
			    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vulkanDevice);
			gameObject.modelBuffers.emplace_back(std::move(buffer));
			gameObject.modelBuffersMemory.emplace_back(std::move(bufferMem));
			gameObject.modelBuffersMapped.emplace_back(gameObject.modelBuffersMemory[i].mapMemory(0, bufferSize));
		}
	};
};