#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "../../DescriptorHandler.hpp"
#include "../../VulkanDevice.hpp"
#include "../Managers/SSBOs.hpp"

struct ModelSSBOsComponent
{
	SSBOs* ssbos;

	void initGlobalSSBO(VulkanDevice& vulkanDevice)
	{
		const size_t MAX_OBJECTS = 10000;
		vk::DeviceSize bufferSize = sizeof(ModelUBO) * MAX_OBJECTS;

		ssbos->modelSSBOs.clear();
		ssbos->modelSSBOsMemory.clear();
		ssbos->modelSSBOsMapped.clear();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			auto [buffer, bufferMem] = VulkanUtils::createBuffer(
			    bufferSize,
			    vk::BufferUsageFlagBits::eStorageBuffer, // Только Storage Buffer
			    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vulkanDevice);

			ssbos->modelSSBOs.emplace_back(std::move(buffer));
			ssbos->modelSSBOsMemory.emplace_back(std::move(bufferMem));
			ssbos->modelSSBOsMapped.emplace_back(ssbos->modelSSBOsMemory[i].mapMemory(0, bufferSize));
		}
	}
};