#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vector>

class PipelineBuilder
{
public:
	PipelineBuilder();

	// Clear all state for a new pipeline
	void clear();

	PipelineBuilder& addShaderStage(vk::ShaderStageFlagBits stage, vk::raii::ShaderModule& shaderModule,
	                                const char* pName = "main",
	                                const vk::SpecializationInfo* pSpecializationInfo = nullptr);

	PipelineBuilder& setVertexInput(const vk::VertexInputBindingDescription* bindingDesc = nullptr,
	                                uint32_t bindingCount = 0,
	                                const vk::VertexInputAttributeDescription* attrDescs = nullptr,
	                                uint32_t attrCount = 0);

	PipelineBuilder& setInputAssembly(vk::PrimitiveTopology topology, bool primitiveRestartEnable = false);

	// Viewport & Scissor (Dynamic state by default)
	PipelineBuilder& setViewportState(uint32_t viewportCount = 1, uint32_t scissorCount = 1);
	PipelineBuilder& addDynamicState(vk::DynamicState state);

	PipelineBuilder& setRasterizer(vk::PolygonMode polygonMode = vk::PolygonMode::eFill,
	                               vk::CullModeFlags cullMode = vk::CullModeFlagBits::eNone,
	                               vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise);

	PipelineBuilder& setMultisampling(vk::SampleCountFlagBits rasterizationSamples = vk::SampleCountFlagBits::e1);

	// Calling this adds one attachment
	PipelineBuilder& addColorBlendAttachment(
	    bool blendEnable = false, vk::BlendFactor srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
	    vk::BlendFactor dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
	    vk::BlendOp colorBlendOp = vk::BlendOp::eAdd, vk::BlendFactor srcAlphaBlendFactor = vk::BlendFactor::eOne,
	    vk::BlendFactor dstAlphaBlendFactor = vk::BlendFactor::eZero, vk::BlendOp alphaBlendOp = vk::BlendOp::eAdd,
	    vk::ColorComponentFlags colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
	                                             vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

	// Shared Color Blending State
	PipelineBuilder& setColorBlendingLogicOp(bool logicOpEnable = false, vk::LogicOp logicOp = vk::LogicOp::eCopy);

	PipelineBuilder& setDepthStencil(bool depthTestEnable = false, bool depthWriteEnable = false,
	                                 vk::CompareOp depthCompareOp = vk::CompareOp::eAlways);

	PipelineBuilder& setPipelineRendering(const std::vector<vk::Format>& colorAttachmentFormats,
	                                      vk::Format depthAttachmentFormat = vk::Format::eUndefined,
	                                      vk::Format stencilAttachmentFormat = vk::Format::eUndefined);

	vk::raii::Pipeline buildGraphics(vk::raii::Device& device, const vk::raii::PipelineLayout& pipelineLayout);

	// Assumes only 1 shader stage was added via addShaderStage
	vk::raii::Pipeline buildCompute(vk::raii::Device& device, const vk::raii::PipelineLayout& pipelineLayout);

private:
	std::vector<vk::PipelineShaderStageCreateInfo> m_ShaderStages;
	vk::PipelineVertexInputStateCreateInfo m_VertexInputInfo{};
	vk::PipelineInputAssemblyStateCreateInfo m_InputAssembly{};
	vk::PipelineViewportStateCreateInfo m_ViewportState{};
	vk::PipelineRasterizationStateCreateInfo m_Rasterizer{};
	vk::PipelineMultisampleStateCreateInfo m_Multisampling{};
	vk::PipelineDepthStencilStateCreateInfo m_DepthStencil{};
	vk::PipelineColorBlendStateCreateInfo m_ColorBlending{};
	vk::PipelineRenderingCreateInfo m_PipelineRendering{};

	std::vector<vk::DynamicState> m_DynamicStates;
	std::vector<vk::PipelineColorBlendAttachmentState> m_ColorBlendAttachments;
	std::vector<vk::Format> m_ColorAttachmentFormats;
};
