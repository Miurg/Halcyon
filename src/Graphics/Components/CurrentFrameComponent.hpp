#pragma once

#include <memory>
#include <vulkan/vulkan_raii.hpp>

struct CurrentFrameComponent
{
	uint32_t currentFrame = 0;
};