#include "PipelineFactory.hpp"
#include "../VulkanUtils.hpp"
#include "../Resources/Managers/VertexIndexBuffer.hpp"
#include "../Resources/Managers/Vertex.hpp"

void PipelineFactory::createGraphicsPipeline(VulkanDevice& vulkanDevice, SwapChain& swapChain,
                                             DescriptorManager& descriptorManager, PipelineHandler& pipelineHandler,
                                             TextureManager& tManager)
{
	vk::raii::ShaderModule shaderModule =
	    PipelineFactory::createShaderModule(VulkanUtils::readFile("shaders/shader.spv"), vulkanDevice);

	vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
	vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertShaderStageInfo.module = shaderModule;
	vertShaderStageInfo.pName = "vertMain";

	//ALPHA_TEST_ENABLED = 0 opaque
	vk::SpecializationMapEntry specEntry{0, 0, sizeof(int32_t)};
	int32_t specData = 0;
	vk::SpecializationInfo specInfo{1, &specEntry, sizeof(specData), &specData};

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
	fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragShaderStageInfo.module = shaderModule;
	fragShaderStageInfo.pName = "fragMain";
	fragShaderStageInfo.pSpecializationInfo = &specInfo;
	vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();
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

	// Normals RT: must be identical to color blend state (independentBlend not enabled)
	std::array<vk::PipelineColorBlendAttachmentState, 2> blendAttachments = {colorBlendAttachment, colorBlendAttachment};

	vk::PipelineColorBlendStateCreateInfo colorBlending;
	colorBlending.logicOpEnable = false;
	colorBlending.logicOp = vk::LogicOp::eCopy;
	colorBlending.attachmentCount = static_cast<uint32_t>(blendAttachments.size());
	colorBlending.pAttachments = blendAttachments.data();

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
	std::array<vk::DescriptorSetLayout, 3> setLayouts = {
	    *descriptorManager.globalSetLayout, // Set 0
	    *descriptorManager.modelSetLayout,  // Set 1
	    *descriptorManager.textureSetLayout // Set 2
	};

	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
	pipelineLayoutInfo.pSetLayouts = setLayouts.data();

	pipelineHandler.pipelineLayout = vk::raii::PipelineLayout(vulkanDevice.device, pipelineLayoutInfo);

	std::array<vk::Format, 2> colorFormats = {swapChain.hdrFormat, vk::Format::eR16G16B16A16Sfloat};
	vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo;
	pipelineRenderingCreateInfo.colorAttachmentCount = static_cast<uint32_t>(colorFormats.size());
	pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats.data();
	pipelineRenderingCreateInfo.depthAttachmentFormat = tManager.textures[swapChain.depthTextureHandle.id].format;

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
	pipelineInfo.layout = pipelineHandler.pipelineLayout;
	pipelineInfo.renderPass = nullptr;

	pipelineHandler.graphicsPipeline = vk::raii::Pipeline(vulkanDevice.device, nullptr, pipelineInfo);
}

void PipelineFactory::createAlphaTestPipeline(VulkanDevice& vulkanDevice, SwapChain& swapChain,
                                              DescriptorManager& descriptorManager, PipelineHandler& pipelineHandler,
                                              TextureManager& tManager)
{
	vk::raii::ShaderModule shaderModule =
	    PipelineFactory::createShaderModule(VulkanUtils::readFile("shaders/shader.spv"), vulkanDevice);

	vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
	vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertShaderStageInfo.module = shaderModule;
	vertShaderStageInfo.pName = "vertMain";

	//ALPHA_TEST_ENABLED = 1 alpha test
	vk::SpecializationMapEntry specEntry{0, 0, sizeof(int32_t)};
	int32_t specData = 1;
	vk::SpecializationInfo specInfo{1, &specEntry, sizeof(specData), &specData};

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
	fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragShaderStageInfo.module = shaderModule;
	fragShaderStageInfo.pName = "fragMain";
	fragShaderStageInfo.pSpecializationInfo = &specInfo;
	vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();
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

	std::array<vk::PipelineColorBlendAttachmentState, 2> blendAttachments = {colorBlendAttachment, colorBlendAttachment};

	vk::PipelineColorBlendStateCreateInfo colorBlending;
	colorBlending.logicOpEnable = false;
	colorBlending.logicOp = vk::LogicOp::eCopy;
	colorBlending.attachmentCount = static_cast<uint32_t>(blendAttachments.size());
	colorBlending.pAttachments = blendAttachments.data();

	std::array<vk::Format, 2> colorFormats = {swapChain.hdrFormat, vk::Format::eR16G16B16A16Sfloat};
	vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo;
	pipelineRenderingCreateInfo.colorAttachmentCount = static_cast<uint32_t>(colorFormats.size());
	pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats.data();
	pipelineRenderingCreateInfo.depthAttachmentFormat = tManager.textures[swapChain.depthTextureHandle.id].format;

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
	pipelineInfo.layout = pipelineHandler.pipelineLayout; // reuse same layout
	pipelineInfo.renderPass = nullptr;

	pipelineHandler.alphaTestPipeline = vk::raii::Pipeline(vulkanDevice.device, nullptr, pipelineInfo);
}

void PipelineFactory::createShadowPipeline(VulkanDevice& vulkanDevice, SwapChain& swapChain,
                                           DescriptorManager& descriptorManager, PipelineHandler& pipelineHandler,
                                           TextureManager& tManager)
{
	vk::raii::ShaderModule shaderModule =
	    PipelineFactory::createShaderModule(VulkanUtils::readFile("shaders/shadow.spv"), vulkanDevice);

	vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
	vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertShaderStageInfo.module = shaderModule;
	vertShaderStageInfo.pName = "vertShadow";

	vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo};

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();
	vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	std::vector dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor, vk::DynamicState::eDepthBias};
	vk::PipelineDynamicStateCreateInfo dynamicState;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
	inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;

	vk::Viewport viewport{0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f};
	vk::Rect2D scissor{vk::Offset2D{0, 0}, {1, 1}};

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
	rasterizer.depthBiasEnable = vk::False;
	rasterizer.cullMode = vk::CullModeFlagBits::eBack;
	rasterizer.frontFace = vk::FrontFace::eCounterClockwise;

	vk::PipelineMultisampleStateCreateInfo multisampling;
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
	multisampling.sampleShadingEnable = false;

	vk::PipelineColorBlendStateCreateInfo colorBlending;
	colorBlending.logicOpEnable = false;
	colorBlending.logicOp = vk::LogicOp::eCopy;
	colorBlending.attachmentCount = 0;
	colorBlending.pAttachments = nullptr;

	vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo;
	pipelineRenderingCreateInfo.colorAttachmentCount = 0;
	pipelineRenderingCreateInfo.pColorAttachmentFormats = nullptr;
	pipelineRenderingCreateInfo.depthAttachmentFormat = tManager.textures[swapChain.depthTextureHandle.id].format;

	vk::PipelineDepthStencilStateCreateInfo depthStencil;
	depthStencil.depthTestEnable = vk::True;
	depthStencil.depthWriteEnable = vk::True;
	depthStencil.depthCompareOp = vk::CompareOp::eGreater;
	depthStencil.depthBoundsTestEnable = vk::False;
	depthStencil.stencilTestEnable = vk::False;

	vk::GraphicsPipelineCreateInfo pipelineInfo;
	pipelineInfo.pNext = &pipelineRenderingCreateInfo;
	pipelineInfo.stageCount = 1;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineHandler.pipelineLayout;
	pipelineInfo.renderPass = nullptr;

	pipelineHandler.shadowPipeline = vk::raii::Pipeline(vulkanDevice.device, nullptr, pipelineInfo);
}

[[nodiscard]] vk::raii::ShaderModule PipelineFactory::createShaderModule(const std::vector<char>& code,
                                                                         VulkanDevice& vulkanDevice)
{
	vk::ShaderModuleCreateInfo createInfo;
	createInfo.codeSize = code.size() * sizeof(char);
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	vk::raii::ShaderModule shaderModule{vulkanDevice.device, createInfo};
	return shaderModule;
}

void PipelineFactory::createFxaaPipeline(VulkanDevice& vulkanDevice, SwapChain& swapChain,
                                         DescriptorManager& descriptorManager, PipelineHandler& pipelineHandler)
{
	vk::raii::ShaderModule shaderModule =
	    PipelineFactory::createShaderModule(VulkanUtils::readFile("shaders/fxaa.spv"), vulkanDevice);

	vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertShaderStageInfo.module = shaderModule;
	vertShaderStageInfo.pName = "vertMain";

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragShaderStageInfo.module = shaderModule;
	fragShaderStageInfo.pName = "fragMain";

	vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssembly.primitiveRestartEnable = vk::False;

	vk::PipelineViewportStateCreateInfo viewportState{};
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
	vk::PipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	vk::PipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.depthClampEnable = vk::False;
	rasterizer.rasterizerDiscardEnable = vk::False;
	rasterizer.polygonMode = vk::PolygonMode::eFill;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = vk::CullModeFlagBits::eNone;
	rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
	rasterizer.depthBiasEnable = vk::False;

	vk::PipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sampleShadingEnable = vk::False;
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

	vk::PipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.depthTestEnable = vk::False;
	depthStencil.depthWriteEnable = vk::False;
	depthStencil.depthCompareOp = vk::CompareOp::eAlways;
	depthStencil.depthBoundsTestEnable = vk::False;
	depthStencil.stencilTestEnable = vk::False;

	vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
	                                      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	colorBlendAttachment.blendEnable = vk::False;

	vk::PipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.logicOpEnable = vk::False;
	colorBlending.logicOp = vk::LogicOp::eCopy;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	vk::PushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eFragment;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(float) * 2; // float2 rcpFrame

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &*descriptorManager.screenSpaceSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	pipelineHandler.fxaaPipelineLayout = vk::raii::PipelineLayout(vulkanDevice.device, pipelineLayoutInfo);

	vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
	pipelineRenderingCreateInfo.colorAttachmentCount = 1;
	pipelineRenderingCreateInfo.pColorAttachmentFormats = &swapChain.swapChainImageFormat;

	vk::GraphicsPipelineCreateInfo pipelineInfo{};
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
	pipelineInfo.layout = pipelineHandler.fxaaPipelineLayout;
	pipelineInfo.renderPass = nullptr;

	pipelineHandler.fxaaPipeline = vk::raii::Pipeline(vulkanDevice.device, nullptr, pipelineInfo);
}

void PipelineFactory::createSsaoPipeline(VulkanDevice& vulkanDevice, SwapChain& swapChain,
                                         DescriptorManager& descriptorManager, PipelineHandler& pipelineHandler)
{
	vk::raii::ShaderModule shaderModule =
	    PipelineFactory::createShaderModule(VulkanUtils::readFile("shaders/ssao.spv"), vulkanDevice);

	vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertShaderStageInfo.module = shaderModule;
	vertShaderStageInfo.pName = "vertMain";

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragShaderStageInfo.module = shaderModule;
	fragShaderStageInfo.pName = "fragMain";

	vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssembly.primitiveRestartEnable = vk::False;

	vk::PipelineViewportStateCreateInfo viewportState{};
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
	vk::PipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	vk::PipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.depthClampEnable = vk::False;
	rasterizer.rasterizerDiscardEnable = vk::False;
	rasterizer.polygonMode = vk::PolygonMode::eFill;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = vk::CullModeFlagBits::eNone;
	rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
	rasterizer.depthBiasEnable = vk::False;

	vk::PipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sampleShadingEnable = vk::False;
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

	vk::PipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.depthTestEnable = vk::False;
	depthStencil.depthWriteEnable = vk::False;

	vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
	                                      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	colorBlendAttachment.blendEnable = vk::False;

	vk::PipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.logicOpEnable = vk::False;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	vk::PushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eFragment;
	pushConstantRange.offset = 0;
	pushConstantRange.size =
	    24; // int kernelSize + float radius + float bias + float power + int numDirections + float maxScreenRadius

	std::array<vk::DescriptorSetLayout, 2> ssaoSetLayouts = {*descriptorManager.screenSpaceSetLayout,
	                                                         *descriptorManager.globalSetLayout};

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(ssaoSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = ssaoSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	pipelineHandler.ssaoPipelineLayout = vk::raii::PipelineLayout(vulkanDevice.device, pipelineLayoutInfo);

	vk::Format ssaoFormat = vk::Format::eR8Unorm;
	vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
	pipelineRenderingCreateInfo.colorAttachmentCount = 1;
	pipelineRenderingCreateInfo.pColorAttachmentFormats = &ssaoFormat;

	vk::GraphicsPipelineCreateInfo pipelineInfo{};
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
	pipelineInfo.layout = pipelineHandler.ssaoPipelineLayout;
	pipelineInfo.renderPass = nullptr;

	pipelineHandler.ssaoPipeline = vk::raii::Pipeline(vulkanDevice.device, nullptr, pipelineInfo);
}

void PipelineFactory::createSsaoBlurPipeline(VulkanDevice& vulkanDevice, SwapChain& swapChain,
                                             DescriptorManager& descriptorManager, PipelineHandler& pipelineHandler)
{
	vk::raii::ShaderModule shaderModule =
	    PipelineFactory::createShaderModule(VulkanUtils::readFile("shaders/ssao_blur.spv"), vulkanDevice);

	vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertShaderStageInfo.module = shaderModule;
	vertShaderStageInfo.pName = "vertMain";

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragShaderStageInfo.module = shaderModule;
	fragShaderStageInfo.pName = "fragMain";

	vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssembly.primitiveRestartEnable = vk::False;

	vk::PipelineViewportStateCreateInfo viewportState{};
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
	vk::PipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	vk::PipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.depthClampEnable = vk::False;
	rasterizer.rasterizerDiscardEnable = vk::False;
	rasterizer.polygonMode = vk::PolygonMode::eFill;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = vk::CullModeFlagBits::eNone;
	rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
	rasterizer.depthBiasEnable = vk::False;

	vk::PipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sampleShadingEnable = vk::False;
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

	vk::PipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.depthTestEnable = vk::False;
	depthStencil.depthWriteEnable = vk::False;

	vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
	                                      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	colorBlendAttachment.blendEnable = vk::False;

	vk::PipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.logicOpEnable = vk::False;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	vk::PushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eFragment;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(float) * 2; // float2 texelSize

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &*descriptorManager.screenSpaceSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	pipelineHandler.ssaoBlurPipelineLayout = vk::raii::PipelineLayout(vulkanDevice.device, pipelineLayoutInfo);

	vk::Format ssaoFormat = vk::Format::eR8Unorm;
	vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
	pipelineRenderingCreateInfo.colorAttachmentCount = 1;
	pipelineRenderingCreateInfo.pColorAttachmentFormats = &ssaoFormat;

	vk::GraphicsPipelineCreateInfo pipelineInfo{};
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
	pipelineInfo.layout = pipelineHandler.ssaoBlurPipelineLayout;
	pipelineInfo.renderPass = nullptr;

	pipelineHandler.ssaoBlurPipeline = vk::raii::Pipeline(vulkanDevice.device, nullptr, pipelineInfo);
}

void PipelineFactory::createCullingPipeline(VulkanDevice& vulkanDevice, DescriptorManager& descriptorManager,
                                            PipelineHandler& pipelineHandler)
{
	vk::raii::ShaderModule shaderModule =
	    PipelineFactory::createShaderModule(VulkanUtils::readFile("shaders/frustum_culling.spv"), vulkanDevice);

	vk::PipelineShaderStageCreateInfo computeShaderStageInfo;
	computeShaderStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
	computeShaderStageInfo.module = *shaderModule;
	computeShaderStageInfo.pName = "computeMain";

	vk::PushConstantRange pushConstantRange;
	pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eCompute;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(uint32_t) * 1;

	vk::DescriptorSetLayout setLayouts[] = {
	    *descriptorManager.globalSetLayout, // Set 0
	    *descriptorManager.modelSetLayout   // Set 1
	};

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
	pipelineLayoutInfo.setLayoutCount = 2;
	pipelineLayoutInfo.pSetLayouts = setLayouts;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	pipelineHandler.cullingPipelineLayout = vk::raii::PipelineLayout(vulkanDevice.device, pipelineLayoutInfo);

	vk::ComputePipelineCreateInfo pipelineInfo;
	pipelineInfo.stage = computeShaderStageInfo;
	pipelineInfo.layout = *pipelineHandler.cullingPipelineLayout;

	pipelineHandler.cullingPipeline = vk::raii::Pipeline(vulkanDevice.device, nullptr, pipelineInfo);
}

void PipelineFactory::createEquirectToCubePipeline(VulkanDevice& vulkanDevice, DescriptorManager& descriptorManager,
                                                   PipelineHandler& pipelineHandler)
{
	vk::raii::ShaderModule shaderModule =
	    PipelineFactory::createShaderModule(VulkanUtils::readFile("shaders/equirect_to_cube.spv"), vulkanDevice);

	vk::PipelineShaderStageCreateInfo computeShaderStageInfo;
	computeShaderStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
	computeShaderStageInfo.module = *shaderModule;
	computeShaderStageInfo.pName = "computeMain";

	vk::PushConstantRange pushConstantRange;
	pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eCompute;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(uint32_t) * 1; // hdrTextureIndex

	vk::DescriptorSetLayout setLayouts[] = {
	    *descriptorManager.textureSetLayout // Set 0: Reusing bindless texture layout for simplicity or creating a new
	                                        // compute pipeline layout based on existing bindings
	};

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = setLayouts;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	pipelineHandler.equirectToCubePipelineLayout = vk::raii::PipelineLayout(vulkanDevice.device, pipelineLayoutInfo);

	vk::ComputePipelineCreateInfo pipelineInfo;
	pipelineInfo.stage = computeShaderStageInfo;
	pipelineInfo.layout = *pipelineHandler.equirectToCubePipelineLayout;

	pipelineHandler.equirectToCubePipeline = vk::raii::Pipeline(vulkanDevice.device, nullptr, pipelineInfo);
}

void PipelineFactory::createSkyboxPipeline(VulkanDevice& vulkanDevice, SwapChain& swapChain,
                                           PipelineHandler& pipelineHandler, TextureManager& tManager)
{
	vk::raii::ShaderModule shaderModule =
	    PipelineFactory::createShaderModule(VulkanUtils::readFile("shaders/skybox.spv"), vulkanDevice);

	vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
	vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertShaderStageInfo.module = *shaderModule;
	vertShaderStageInfo.pName = "vertMain";

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
	fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragShaderStageInfo.module = *shaderModule;
	fragShaderStageInfo.pName = "fragMain";

	vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

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
	rasterizer.cullMode = vk::CullModeFlagBits::eNone;
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

	std::array<vk::PipelineColorBlendAttachmentState, 2> blendAttachments = {colorBlendAttachment, colorBlendAttachment};

	vk::PipelineColorBlendStateCreateInfo colorBlending;
	colorBlending.logicOpEnable = false;
	colorBlending.logicOp = vk::LogicOp::eCopy;
	colorBlending.attachmentCount = static_cast<uint32_t>(blendAttachments.size());
	colorBlending.pAttachments = blendAttachments.data();

	std::array<vk::Format, 2> colorFormats = {swapChain.hdrFormat, vk::Format::eR16G16B16A16Sfloat};
	vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo;
	pipelineRenderingCreateInfo.colorAttachmentCount = static_cast<uint32_t>(colorFormats.size());
	pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats.data();
	pipelineRenderingCreateInfo.depthAttachmentFormat = tManager.textures[swapChain.depthTextureHandle.id].format;

	vk::PipelineDepthStencilStateCreateInfo depthStencil;
	depthStencil.depthTestEnable = vk::True;
	depthStencil.depthWriteEnable = vk::False;
	depthStencil.depthCompareOp = vk::CompareOp::eLessOrEqual;
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
	// Reuse existing pipeline layout from the opaque/alpha pass
	pipelineInfo.layout = *pipelineHandler.pipelineLayout;
	pipelineInfo.renderPass = nullptr;

	pipelineHandler.skyboxPipeline = vk::raii::Pipeline(vulkanDevice.device, nullptr, pipelineInfo);
}
