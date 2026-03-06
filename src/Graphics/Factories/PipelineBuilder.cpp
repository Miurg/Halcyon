#include "PipelineBuilder.hpp"

PipelineBuilder::PipelineBuilder()
{
	clear();
}

void PipelineBuilder::clear()
{
	m_ShaderStages.clear();

	m_VertexInputInfo = vk::PipelineVertexInputStateCreateInfo{};

	m_InputAssembly = vk::PipelineInputAssemblyStateCreateInfo{};
	m_InputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
	m_InputAssembly.primitiveRestartEnable = false;

	m_ViewportState = vk::PipelineViewportStateCreateInfo{};
	m_ViewportState.viewportCount = 1;
	m_ViewportState.scissorCount = 1;

	m_Rasterizer = vk::PipelineRasterizationStateCreateInfo{};
	m_Rasterizer.depthClampEnable = false;
	m_Rasterizer.rasterizerDiscardEnable = false;
	m_Rasterizer.polygonMode = vk::PolygonMode::eFill;
	m_Rasterizer.lineWidth = 1.0f;
	m_Rasterizer.cullMode = vk::CullModeFlagBits::eNone;
	m_Rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
	m_Rasterizer.depthBiasEnable = false;

	m_Multisampling = vk::PipelineMultisampleStateCreateInfo{};
	m_Multisampling.sampleShadingEnable = false;
	m_Multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

	m_DepthStencil = vk::PipelineDepthStencilStateCreateInfo{};
	m_DepthStencil.depthTestEnable = false;
	m_DepthStencil.depthWriteEnable = false;
	m_DepthStencil.depthCompareOp = vk::CompareOp::eAlways;
	m_DepthStencil.depthBoundsTestEnable = false;
	m_DepthStencil.stencilTestEnable = false;

	m_ColorBlending = vk::PipelineColorBlendStateCreateInfo{};
	m_ColorBlending.logicOpEnable = false;
	m_ColorBlending.logicOp = vk::LogicOp::eCopy;

	m_DynamicStates.clear();
	m_ColorBlendAttachments.clear();
	m_ColorAttachmentFormats.clear();

	m_PipelineRendering = vk::PipelineRenderingCreateInfo{};
}

PipelineBuilder& PipelineBuilder::addShaderStage(vk::ShaderStageFlagBits stage, vk::raii::ShaderModule& shaderModule,
                                                 const char* pName, const vk::SpecializationInfo* pSpecializationInfo)
{
	vk::PipelineShaderStageCreateInfo shaderStageInfo{};
	shaderStageInfo.stage = stage;
	shaderStageInfo.module = *shaderModule;
	shaderStageInfo.pName = pName;
	shaderStageInfo.pSpecializationInfo = pSpecializationInfo;
	m_ShaderStages.push_back(shaderStageInfo);
	return *this;
}

PipelineBuilder& PipelineBuilder::setVertexInput(const vk::VertexInputBindingDescription* bindingDesc,
                                                 uint32_t bindingCount,
                                                 const vk::VertexInputAttributeDescription* attrDescs,
                                                 uint32_t attrCount)
{
	m_VertexInputInfo.vertexBindingDescriptionCount = bindingCount;
	m_VertexInputInfo.pVertexBindingDescriptions = bindingDesc;
	m_VertexInputInfo.vertexAttributeDescriptionCount = attrCount;
	m_VertexInputInfo.pVertexAttributeDescriptions = attrDescs;
	return *this;
}

PipelineBuilder& PipelineBuilder::setInputAssembly(vk::PrimitiveTopology topology, bool primitiveRestartEnable)
{
	m_InputAssembly.topology = topology;
	m_InputAssembly.primitiveRestartEnable = primitiveRestartEnable;
	return *this;
}

PipelineBuilder& PipelineBuilder::setViewportState(uint32_t viewportCount, uint32_t scissorCount)
{
	m_ViewportState.viewportCount = viewportCount;
	m_ViewportState.scissorCount = scissorCount;
	return *this;
}

PipelineBuilder& PipelineBuilder::addDynamicState(vk::DynamicState state)
{
	m_DynamicStates.push_back(state);
	return *this;
}

PipelineBuilder& PipelineBuilder::setRasterizer(vk::PolygonMode polygonMode, vk::CullModeFlags cullMode,
                                                vk::FrontFace frontFace)
{
	m_Rasterizer.polygonMode = polygonMode;
	m_Rasterizer.cullMode = cullMode;
	m_Rasterizer.frontFace = frontFace;
	return *this;
}

PipelineBuilder& PipelineBuilder::setMultisampling(vk::SampleCountFlagBits rasterizationSamples)
{
	m_Multisampling.rasterizationSamples = rasterizationSamples;
	return *this;
}

PipelineBuilder& PipelineBuilder::addColorBlendAttachment(bool blendEnable, vk::BlendFactor srcColorBlendFactor,
                                                          vk::BlendFactor dstColorBlendFactor, vk::BlendOp colorBlendOp,
                                                          vk::BlendFactor srcAlphaBlendFactor,
                                                          vk::BlendFactor dstAlphaBlendFactor, vk::BlendOp alphaBlendOp,
                                                          vk::ColorComponentFlags colorWriteMask)
{
	vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = colorWriteMask;
	colorBlendAttachment.blendEnable = blendEnable;
	colorBlendAttachment.srcColorBlendFactor = srcColorBlendFactor;
	colorBlendAttachment.dstColorBlendFactor = dstColorBlendFactor;
	colorBlendAttachment.colorBlendOp = colorBlendOp;
	colorBlendAttachment.srcAlphaBlendFactor = srcAlphaBlendFactor;
	colorBlendAttachment.dstAlphaBlendFactor = dstAlphaBlendFactor;
	colorBlendAttachment.alphaBlendOp = alphaBlendOp;
	m_ColorBlendAttachments.push_back(colorBlendAttachment);
	return *this;
}

PipelineBuilder& PipelineBuilder::setColorBlendingLogicOp(bool logicOpEnable, vk::LogicOp logicOp)
{
	m_ColorBlending.logicOpEnable = logicOpEnable;
	m_ColorBlending.logicOp = logicOp;
	return *this;
}

PipelineBuilder& PipelineBuilder::setDepthStencil(bool depthTestEnable, bool depthWriteEnable,
                                                  vk::CompareOp depthCompareOp)
{
	m_DepthStencil.depthTestEnable = depthTestEnable;
	m_DepthStencil.depthWriteEnable = depthWriteEnable;
	m_DepthStencil.depthCompareOp = depthCompareOp;
	return *this;
}

PipelineBuilder& PipelineBuilder::setPipelineRendering(const std::vector<vk::Format>& colorAttachmentFormats,
                                                       vk::Format depthAttachmentFormat,
                                                       vk::Format stencilAttachmentFormat)
{
	m_ColorAttachmentFormats = colorAttachmentFormats;
	m_PipelineRendering.colorAttachmentCount = static_cast<uint32_t>(m_ColorAttachmentFormats.size());
	m_PipelineRendering.pColorAttachmentFormats =
	    m_ColorAttachmentFormats.empty() ? nullptr : m_ColorAttachmentFormats.data();
	m_PipelineRendering.depthAttachmentFormat = depthAttachmentFormat;
	m_PipelineRendering.stencilAttachmentFormat = stencilAttachmentFormat;
	return *this;
}

vk::raii::Pipeline PipelineBuilder::buildGraphics(vk::raii::Device& device,
                                                  const vk::raii::PipelineLayout& pipelineLayout)
{
	vk::PipelineDynamicStateCreateInfo dynamicStateInfo{};
	dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(m_DynamicStates.size());
	dynamicStateInfo.pDynamicStates = m_DynamicStates.data();

	m_ColorBlending.attachmentCount = static_cast<uint32_t>(m_ColorBlendAttachments.size());
	m_ColorBlending.pAttachments = m_ColorBlendAttachments.empty() ? nullptr : m_ColorBlendAttachments.data();

	vk::GraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.pNext = &m_PipelineRendering;
	pipelineInfo.stageCount = static_cast<uint32_t>(m_ShaderStages.size());
	pipelineInfo.pStages = m_ShaderStages.data();
	pipelineInfo.pVertexInputState = &m_VertexInputInfo;
	pipelineInfo.pInputAssemblyState = &m_InputAssembly;
	pipelineInfo.pViewportState = &m_ViewportState;
	pipelineInfo.pDepthStencilState = &m_DepthStencil;
	pipelineInfo.pRasterizationState = &m_Rasterizer;
	pipelineInfo.pMultisampleState = &m_Multisampling;
	pipelineInfo.pColorBlendState = &m_ColorBlending;
	pipelineInfo.pDynamicState = &dynamicStateInfo;
	pipelineInfo.layout = *pipelineLayout;
	pipelineInfo.renderPass = nullptr; // Using dynamic rendering
	pipelineInfo.basePipelineHandle = nullptr;

	return vk::raii::Pipeline(device, nullptr, pipelineInfo);
}

vk::raii::Pipeline PipelineBuilder::buildCompute(vk::raii::Device& device,
                                                 const vk::raii::PipelineLayout& pipelineLayout)
{
	vk::ComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.stage = m_ShaderStages[0];
	pipelineInfo.layout = *pipelineLayout;
	pipelineInfo.basePipelineHandle = nullptr;

	return vk::raii::Pipeline(device, nullptr, pipelineInfo);
}
