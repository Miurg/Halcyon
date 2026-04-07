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
BuiltPipeline PipelineFactory::build(vk::raii::Device& device, const PipelineDescription& desc,
                                      const std::vector<vk::DescriptorSetLayout>& resolvedLayouts)
{
	auto shader = loadShader(device, desc.shaderPath);

	// Specialization constants (index == constant_id)
	std::vector<vk::SpecializationMapEntry> specEntries;
	vk::SpecializationInfo specInfo{};
	if (!desc.specializationValues.empty())
	{
		for (uint32_t i = 0; i < desc.specializationValues.size(); i++)
			specEntries.push_back({i, i * static_cast<uint32_t>(sizeof(int32_t)), sizeof(int32_t)});
		specInfo.mapEntryCount = static_cast<uint32_t>(specEntries.size());
		specInfo.pMapEntries   = specEntries.data();
		specInfo.dataSize      = desc.specializationValues.size() * sizeof(int32_t);
		specInfo.pData         = desc.specializationValues.data();
	}

	PipelineBuilder builder;
	if (desc.isCompute)
	{
		if (!desc.computeEntry.empty())
		{
			builder.addShaderStage(vk::ShaderStageFlagBits::eCompute, shader, desc.computeEntry.c_str());
		}
	}
	else
	{
		if (!desc.vertEntry.empty())
		{
			builder.addShaderStage(vk::ShaderStageFlagBits::eVertex, shader, desc.vertEntry.c_str());
		}

		if (!desc.fragEntry.empty())
		{
			builder.addShaderStage(vk::ShaderStageFlagBits::eFragment, shader, desc.fragEntry.c_str(),
			                       !desc.specializationValues.empty() ? &specInfo : nullptr);
		}
		if (!desc.vertexBindings.empty())
		{
			builder.setVertexInput(desc.vertexBindings.data(), static_cast<uint32_t>(desc.vertexBindings.size()), desc.vertexAttributes.data(), static_cast<uint32_t>(desc.vertexAttributes.size()));
		}
		else
		{
			builder.setVertexInput(nullptr, 0, nullptr, 0);
		}

		builder.setInputAssembly(desc.topology)
		    .setViewportState(1, 1)
		    .addDynamicState(vk::DynamicState::eViewport)
		    .addDynamicState(vk::DynamicState::eScissor)
		    .addDynamicState(vk::DynamicState::eCullMode)
		    .setRasterizer(vk::PolygonMode::eFill, desc.cullMode, desc.frontFace)
		    .setMultisampling(desc.rasterizationSamples);

		for (const auto& att : desc.colorAttachments)
		{
			builder.addColorBlendAttachment(att.blendEnable, att.srcColor, att.dstColor, att.colorOp, att.srcAlpha,
			                                att.dstAlpha, att.alphaOp, att.writeMask);
		}

		builder.setColorBlendingLogicOp(false, vk::LogicOp::eCopy)
		    .setDepthStencil(desc.depthTest, desc.depthWrite, desc.depthOp)
		    .setPipelineRendering(desc.colorFormats, desc.depthFormat, desc.stencilFormat);
	}
	

	BuiltPipeline result;
	result.layout = buildLayout(device, resolvedLayouts, desc.pushConstants);
	if (desc.isCompute)
	{
		result.pipeline = builder.buildCompute(device, result.layout);
	}
	else
	{
		result.pipeline = builder.buildGraphics(device, result.layout);
	}
	
	return result;
}