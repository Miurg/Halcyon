#pragma once

#include "GraphicsCore/FrameData.hpp"
#include <vector>

class FrameManager
{
public:
	int initFrameData();
	FrameManager(VulkanDevice& vulkanDevice);
	~FrameManager();
	std::vector<FrameData> frames;

private:
	VulkanDevice& vulkanDevice;
};