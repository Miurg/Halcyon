#pragma once

#include <vulkan/vulkan_raii.hpp>

struct ObjectDSetComponent
{
	vk::DescriptorSet ssboDSet[MAX_FRAMES_IN_FLIGHT];

	int ssboBufer = -1;
};