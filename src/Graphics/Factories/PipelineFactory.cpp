#include "PipelineFactory.hpp"
#include "../VulkanUtils.hpp"
#include "../Resources/Managers/Vertex.hpp"
#include "PipelineBuilder.hpp"

ColorBlendAttachmentDesc PipelineFactory::blendedAttachment()
{
	return ColorBlendAttachmentDesc{
	    .blendEnable = true,
	    .srcColor = vk::BlendFactor::eSrcAlpha,
	    .dstColor = vk::BlendFactor::eOneMinusSrcAlpha,
	    .colorOp = vk::BlendOp::eAdd,
	    .srcAlpha = vk::BlendFactor::eOne,
	    .dstAlpha = vk::BlendFactor::eZero,
	    .alphaOp = vk::BlendOp::eAdd,
	};
}

ColorBlendAttachmentDesc PipelineFactory::opaqueAttachment()
{
	return ColorBlendAttachmentDesc{.blendEnable = false};
}

vk::raii::ShaderModule PipelineFactory::loadShader(vk::raii::Device& device, const std::string& path)
{
	auto code = VulkanUtils::readFile(path);
	vk::ShaderModuleCreateInfo info{};
	info.codeSize = code.size();
	info.pCode = reinterpret_cast<const uint32_t*>(code.data());
	return vk::raii::ShaderModule{device, info};
}

vk::raii::PipelineLayout PipelineFactory::buildLayout(vk::raii::Device& device,
                                                      const std::vector<vk::DescriptorSetLayout>& setLayouts,
                                                      const std::vector<vk::PushConstantRange>& pushConstants)
{
	vk::PipelineLayoutCreateInfo info{};
	info.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
	info.pSetLayouts = setLayouts.empty() ? nullptr : setLayouts.data();
	info.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
	info.pPushConstantRanges = pushConstants.empty() ? nullptr : pushConstants.data();
	return vk::raii::PipelineLayout{device, info};
}

// Graphics build
BuiltPipeline PipelineFactory::build(vk::raii::Device& device, const GraphicsPipelineDesc& desc)
{
	auto shader = loadShader(device, desc.shaderPath);

	// Specialization constant (один int32, slot 0)
	vk::SpecializationMapEntry specEntry{0, 0, sizeof(int32_t)};
	vk::SpecializationInfo specInfo{};
	int32_t specValue = 0;
	if (desc.specializationValue.has_value())
	{
		specValue = *desc.specializationValue;
		specInfo.mapEntryCount = 1;
		specInfo.pMapEntries = &specEntry;
		specInfo.dataSize = sizeof(int32_t);
		specInfo.pData = &specValue;
	}

	PipelineBuilder builder;
	builder.addShaderStage(vk::ShaderStageFlagBits::eVertex, shader, desc.vertEntry.c_str());

	if (!desc.fragEntry.empty())
	{
		builder.addShaderStage(vk::ShaderStageFlagBits::eFragment, shader, desc.fragEntry.c_str(),
		                       desc.specializationValue.has_value() ? &specInfo : nullptr);
	}

	if (desc.vertexBinding)
	{
		builder.setVertexInput(desc.vertexBinding, 1, desc.vertexAttributes, desc.attributeCount);
	}
	else
	{
		builder.setVertexInput(nullptr, 0, nullptr, 0);
	}

	builder.setInputAssembly(vk::PrimitiveTopology::eTriangleList)
	    .setViewportState(1, 1)
	    .addDynamicState(vk::DynamicState::eViewport)
	    .addDynamicState(vk::DynamicState::eScissor)
	    .addDynamicState(vk::DynamicState::eCullMode)
	    .setRasterizer(vk::PolygonMode::eFill, desc.cullMode, desc.frontFace)
	    .setMultisampling(vk::SampleCountFlagBits::e1);

	for (const auto& att : desc.colorAttachments)
	{
		builder.addColorBlendAttachment(att.blendEnable, att.srcColor, att.dstColor, att.colorOp, att.srcAlpha,
		                                att.dstAlpha, att.alphaOp, att.writeMask);
	}

	builder.setColorBlendingLogicOp(false, vk::LogicOp::eCopy)
	    .setDepthStencil(desc.depthTest, desc.depthWrite, desc.depthOp)
	    .setPipelineRendering(desc.colorFormats, desc.depthFormat, desc.stencilFormat);

	BuiltPipeline result;
	result.layout = buildLayout(device, desc.setLayouts, desc.pushConstants);
	result.pipeline = builder.buildGraphics(device, result.layout);
	return result;
}

// Compute build
BuiltPipeline PipelineFactory::build(vk::raii::Device& device, const ComputePipelineDesc& desc)
{
	auto shader = loadShader(device, desc.shaderPath);

	PipelineBuilder builder;
	builder.addShaderStage(vk::ShaderStageFlagBits::eCompute, shader, desc.entry.c_str());

	BuiltPipeline result;
	result.layout = buildLayout(device, desc.setLayouts, desc.pushConstants);
	result.pipeline = builder.buildCompute(device, result.layout);
	return result;
}