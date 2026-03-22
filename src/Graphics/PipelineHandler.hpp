#pragma once

#include <vulkan/vulkan_raii.hpp>

class PipelineHandler
{
public:
	vk::raii::PipelineLayout pipelineLayout = nullptr;
	vk::raii::Pipeline graphicsPipeline = nullptr;
	vk::raii::Pipeline alphaTestPipeline = nullptr;
	vk::raii::Pipeline shadowPipeline = nullptr;
	vk::raii::Pipeline depthPrepassPipeline = nullptr;

	vk::raii::Pipeline fxaaPipeline = nullptr;
	vk::raii::PipelineLayout fxaaPipelineLayout = nullptr;

	vk::raii::Pipeline ssaoPipeline = nullptr;
	vk::raii::PipelineLayout ssaoPipelineLayout = nullptr;

	vk::raii::Pipeline ssaoBlurPipeline = nullptr;
	vk::raii::PipelineLayout ssaoBlurPipelineLayout = nullptr;

	vk::raii::Pipeline ssaoApplyPipeline = nullptr;
	vk::raii::PipelineLayout ssaoApplyPipelineLayout = nullptr;

	vk::raii::Pipeline cullingPipeline = nullptr;
	vk::raii::PipelineLayout cullingPipelineLayout = nullptr;

	vk::raii::Pipeline equirectToCubePipeline = nullptr;
	vk::raii::PipelineLayout equirectToCubePipelineLayout = nullptr;

	vk::raii::Pipeline irradiancePipeline = nullptr;
	vk::raii::PipelineLayout irradiancePipelineLayout = nullptr;

	vk::raii::Pipeline prefilterPipeline = nullptr;
	vk::raii::PipelineLayout prefilterPipelineLayout = nullptr;

	vk::raii::Pipeline brdfLutPipeline = nullptr;
	vk::raii::PipelineLayout brdfLutPipelineLayout = nullptr;

	vk::raii::Pipeline skyboxPipeline = nullptr;

	vk::raii::Pipeline compactingCullPipeline = nullptr;
	vk::raii::PipelineLayout compactingCullPipelineLayout = nullptr;

	vk::raii::Pipeline shadowCullingPipeline = nullptr;
	vk::raii::PipelineLayout shadowCullingPipelineLayout = nullptr;

	vk::raii::Pipeline resetInstancePipeline = nullptr;
	vk::raii::PipelineLayout resetInstancePipelineLayout = nullptr;

	vk::raii::Pipeline toneMappingPipeline = nullptr;
	vk::raii::PipelineLayout toneMappingPipelineLayout = nullptr;
};
