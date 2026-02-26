#pragma once

#include <vulkan/vulkan_raii.hpp>

class PipelineHandler
{
public:
	vk::raii::PipelineLayout pipelineLayout = nullptr;
	vk::raii::Pipeline graphicsPipeline = nullptr;
	vk::raii::Pipeline alphaTestPipeline = nullptr;
	vk::raii::Pipeline shadowPipeline = nullptr;

	vk::raii::Pipeline fxaaPipeline = nullptr;
	vk::raii::PipelineLayout fxaaPipelineLayout = nullptr;

	vk::raii::Pipeline ssaoPipeline = nullptr;
	vk::raii::PipelineLayout ssaoPipelineLayout = nullptr;

	vk::raii::Pipeline ssaoBlurPipeline = nullptr;
	vk::raii::PipelineLayout ssaoBlurPipelineLayout = nullptr;

	vk::raii::Pipeline cullingPipeline = nullptr;
	vk::raii::PipelineLayout cullingPipelineLayout = nullptr;
};
