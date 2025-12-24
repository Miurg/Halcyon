#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan_raii.hpp>
#include "../FrameData.hpp"

struct FrameDataComponent
{
	std::vector<FrameData>* frameDataArray;

	FrameDataComponent(std::vector<FrameData>* frameData) : frameDataArray(frameData) {}
};