#pragma once

#include <vulkan/vulkan_raii.hpp>

class PipelineHandler
{
public:
	vk::raii::PipelineLayout pipelineLayout = nullptr;
	vk::raii::Pipeline graphicsPipeline = nullptr;
};
