#include "PipelineManager.hpp"
#include "VulkanUtils.hpp"

PipelineManager::PipelineManager(VulkanDevice& deviceContext, SwapChain& swapChain, DescriptorHandler& descriptor)
    : vulkanDevice(deviceContext), swapChain(swapChain), descriptorHandler(descriptor)
{
	PipelineManager::createGraphicsPipeline();
}

PipelineManager::~PipelineManager() {}

void PipelineManager::createGraphicsPipeline()
{
	vk::raii::ShaderModule shaderModule = createShaderModule(VulkanUtils::readFile("shaders/shader.spv"));

	vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
	vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertShaderStageInfo.module = shaderModule;
	vertShaderStageInfo.pName = "vertMain";
	vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
	fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragShaderStageInfo.module = shaderModule;
	fragShaderStageInfo.pName = "fragMain";
	vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

	auto bindingDescription = Model::Vertex::getBindingDescription();
	auto attributeDescriptions = Model::Vertex::getAttributeDescriptions();
	vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	std::vector dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
	vk::PipelineDynamicStateCreateInfo dynamicState;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
	inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;

	vk::Viewport viewport{0.0f,
	                      0.0f,
	                      static_cast<float>(swapChain.swapChainExtent.width),
	                      static_cast<float>(swapChain.swapChainExtent.height),
	                      0.0f,
	                      1.0f};
	vk::Rect2D scissor{vk::Offset2D{0, 0}, swapChain.swapChainExtent};

	vk::PipelineViewportStateCreateInfo viewportState;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	vk::PipelineRasterizationStateCreateInfo rasterizer;
	rasterizer.depthClampEnable = false;
	rasterizer.rasterizerDiscardEnable = false;
	rasterizer.polygonMode = vk::PolygonMode::eFill;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = vk::CullModeFlagBits::eBack;
	rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
	rasterizer.depthBiasEnable = false;

	vk::PipelineMultisampleStateCreateInfo multisampling;
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
	multisampling.sampleShadingEnable = false;

	vk::PipelineColorBlendAttachmentState colorBlendAttachment;
	colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
	                                      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	colorBlendAttachment.blendEnable = true;
	colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
	colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
	colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
	colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

	vk::PipelineColorBlendStateCreateInfo colorBlending;
	colorBlending.logicOpEnable = false;
	colorBlending.logicOp = vk::LogicOp::eCopy;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
	pipelineLayoutInfo.setLayoutCount = 1;
	vk::DescriptorSetLayout dslHandle = descriptorHandler.descriptorSetLayout;
	pipelineLayoutInfo.pSetLayouts = &dslHandle;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	pipelineLayout = vk::raii::PipelineLayout(vulkanDevice.device, pipelineLayoutInfo);

	vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo;
	pipelineRenderingCreateInfo.colorAttachmentCount = 1;
	pipelineRenderingCreateInfo.pColorAttachmentFormats = &swapChain.swapChainImageFormat;
	pipelineRenderingCreateInfo.depthAttachmentFormat = swapChain.depthFormat;

	vk::PipelineDepthStencilStateCreateInfo depthStencil;
	depthStencil.depthTestEnable = vk::True;
	depthStencil.depthWriteEnable = vk::True;
	depthStencil.depthCompareOp = vk::CompareOp::eLess;
	depthStencil.depthBoundsTestEnable = vk::False;
	depthStencil.stencilTestEnable = vk::False;

	vk::GraphicsPipelineCreateInfo pipelineInfo;
	pipelineInfo.pNext = &pipelineRenderingCreateInfo;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = nullptr;

	graphicsPipeline = vk::raii::Pipeline(vulkanDevice.device, nullptr, pipelineInfo);
}

[[nodiscard]] vk::raii::ShaderModule PipelineManager::createShaderModule(const std::vector<char>& code) const
{
	vk::ShaderModuleCreateInfo createInfo;
	createInfo.codeSize = code.size() * sizeof(char);
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	vk::raii::ShaderModule shaderModule{vulkanDevice.device, createInfo};
	return shaderModule;
}