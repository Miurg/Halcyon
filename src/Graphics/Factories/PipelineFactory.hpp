#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vector>
#include <string>
#include <optional>

struct ColorBlendAttachmentDesc
{
	bool blendEnable = false;
	vk::BlendFactor srcColor = vk::BlendFactor::eSrcAlpha;
	vk::BlendFactor dstColor = vk::BlendFactor::eOneMinusSrcAlpha;
	vk::BlendOp colorOp = vk::BlendOp::eAdd;
	vk::BlendFactor srcAlpha = vk::BlendFactor::eOne;
	vk::BlendFactor dstAlpha = vk::BlendFactor::eZero;
	vk::BlendOp alphaOp = vk::BlendOp::eAdd;
	vk::ColorComponentFlags writeMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
	                                    vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
};

struct PipelineDescription
{
	bool isCompute = false;
	std::string shaderPath;
	std::string vertEntry = "vertMain";
	std::string fragEntry = "fragMain"; // empty - vertex-only
	std::string computeEntry = "computeMain";
	std::vector<int32_t> specializationValues; // индекс = constant_id

	// Vertex input — empty means "no vertex input" (fullscreen pass)
	std::vector<vk::VertexInputBindingDescription> vertexBindings;
	std::vector<vk::VertexInputAttributeDescription> vertexAttributes;

	vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;

	vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack;
	vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;

	bool depthTest = false;
	bool depthWrite = false;
	vk::CompareOp depthOp = vk::CompareOp::eAlways;

	std::vector<ColorBlendAttachmentDesc> colorAttachments;

	std::vector<vk::Format> colorFormats;
	vk::Format depthFormat = vk::Format::eUndefined;
	vk::Format stencilFormat = vk::Format::eUndefined;

	vk::SampleCountFlagBits rasterizationSamples = vk::SampleCountFlagBits::e1;

	// Layout names — resolved to raw handles by PipelineManager via DescriptorManager
	std::vector<std::string> setLayoutNames;
	std::vector<vk::PushConstantRange> pushConstants;
};

// Result of build - pipeline + layout + desc
struct BuiltPipeline
{
	vk::raii::PipelineLayout layout{nullptr};
	vk::raii::Pipeline pipeline{nullptr};
	PipelineDescription desc;
};

class PipelineFactory
{
public:
	static BuiltPipeline build(vk::raii::Device& device, const PipelineDescription& desc,
	                           const std::vector<vk::DescriptorSetLayout>& resolvedLayouts);

	// Standart blended attachment (HDR + normals RT)
	static ColorBlendAttachmentDesc blendedAttachment();
	// Attachment without blend
	static ColorBlendAttachmentDesc opaqueAttachment();

private:
	static vk::raii::ShaderModule loadShader(vk::raii::Device& device, const std::string& path);
	static vk::raii::PipelineLayout buildLayout(vk::raii::Device& device,
	                                            const std::vector<vk::DescriptorSetLayout>& setLayouts,
	                                            const std::vector<vk::PushConstantRange>& pushConstants);
};