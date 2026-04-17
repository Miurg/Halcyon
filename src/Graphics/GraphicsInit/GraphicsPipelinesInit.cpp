#include "GraphicsPipelinesInit.hpp"
#include "../Factories/PipelineFactory.hpp"
#include <iostream>
#include <vk_mem_alloc.h>
#include "../Resources/Managers/Vertex.hpp"
#include "../Components/VulkanDeviceComponent.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../Components/VMAllocatorComponent.hpp"
#include "../Components/TextureManagerComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Components/RenderGraphComponent.hpp"
#include "../Components/PipelineManagerComponent.hpp"
#include "../RenderGraph/RenderGraph.hpp"
#include "../Components/NameComponent.hpp"
#include "../VulkanDevice.hpp"
#include "../SwapChain.hpp"
#include "../Resources/Managers/TextureManager.hpp"
#include "../Resources/Managers/DescriptorManager.hpp"
#include "../Resources/Managers/Bindings.hpp"
#include "../Managers/PipelineManager.hpp"
#include "../GraphicsContexts.hpp"
#include "../ShaderReloader.hpp"
#include "../Components/ShaderReloaderComponent.hpp"
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>
#include <Orhescyon/Entitys/ActiveEntitySet.hpp>
#include <Orhescyon/GeneralManager.hpp>
#include "../RenderGraph/RGResource.hpp"

#pragma region initPipelines
void GraphicsPipelinesInit::initPipelines(GeneralManager& gm)
{
#ifdef _DEBUG
	std::cout << "GraphicsPipelinesInit::VULKANNEEDS::Start..." << std::endl;
#endif //_DEBUG
	VulkanDevice* vulkanDevice =
	    gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	DescriptorManager* dManager =
	    gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	TextureManager* tManager = gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	VmaAllocator vmaAlloc = gm.getContextComponent<VMAllocatorContext, VMAllocatorComponent>()->allocator;
	SwapChain* swapChain = gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;

#pragma region RenderGraph

	Entity rgEntity = gm.createEntity();
	RenderGraph* rg = new RenderGraph(*vulkanDevice, vmaAlloc);
	gm.registerContext<RenderGraphContext>(rgEntity);
	gm.addComponent<RenderGraphComponent>(rgEntity, rg);
	gm.addComponent<NameComponent>(rgEntity, "SYSTEM Render Graph");

	rg->handleResize(swapChain->swapChainExtent.width, swapChain->swapChainExtent.height);
	vk::SampleCountFlags counts = static_cast<vk::SampleCountFlags>(vulkanDevice->maxMsaaSamples);
	vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e4;

	// Prefer 4x MSAA if available, but allow fallback to 2x or 1x if not supported
	for (auto samples : {vk::SampleCountFlagBits::e4, vk::SampleCountFlagBits::e2, vk::SampleCountFlagBits::e1})
	{
		if (counts & samples)
		{
			msaaSamples = samples;
			break;
		}
	}

	// Logical Streams
	rg->declareLogicalStream(
	    "DepthMSAA", {tManager->findBestFormat(), RGSizeMode::FullExtent, vk::ImageAspectFlagBits::eDepth, msaaSamples});
	rg->declareLogicalStream(
	    "MainColorMSAA", {swapChain->hdrFormat, RGSizeMode::FullExtent, vk::ImageAspectFlagBits::eColor, msaaSamples});
	rg->declareLogicalStream("ViewNormalsMSAA", {vk::Format::eR16G16B16A16Sfloat, RGSizeMode::FullExtent,
	                                             vk::ImageAspectFlagBits::eColor, msaaSamples});

	rg->declareLogicalStream("Depth",
	                         {tManager->findBestFormat(), RGSizeMode::FullExtent, vk::ImageAspectFlagBits::eDepth});
	rg->declareLogicalStream("MainColor",
	                         {swapChain->hdrFormat, RGSizeMode::FullExtent, vk::ImageAspectFlagBits::eColor});
	rg->declareLogicalStream("ViewNormals",
	                         {vk::Format::eR16G16B16A16Sfloat, RGSizeMode::FullExtent, vk::ImageAspectFlagBits::eColor});
	rg->declareLogicalStream("SSAOTexture",
	                         {vk::Format::eR8Unorm, RGSizeMode::HalfExtent, vk::ImageAspectFlagBits::eColor});
	rg->declareLogicalStream("SSAOBlurTempTexture",
	                         {vk::Format::eR8Unorm, RGSizeMode::HalfExtent, vk::ImageAspectFlagBits::eColor});
	rg->declareLogicalStream("SSAOBlurTexture",
	                         {vk::Format::eR8Unorm, RGSizeMode::HalfExtent, vk::ImageAspectFlagBits::eColor});
	rg->declareLogicalStream("PostProcessColor",
	                         {swapChain->swapChainImageFormat, RGSizeMode::FullExtent, vk::ImageAspectFlagBits::eColor});
	rg->declareLogicalStream("BloomMip0",
	                         {swapChain->hdrFormat, RGSizeMode::HalfExtent, vk::ImageAspectFlagBits::eColor});
	rg->declareLogicalStream("BloomMip1",
	                         {swapChain->hdrFormat, RGSizeMode::QuarterExtent, vk::ImageAspectFlagBits::eColor});
	rg->declareLogicalStream("BloomMip2",
	                         {swapChain->hdrFormat, RGSizeMode::EighthExtent, vk::ImageAspectFlagBits::eColor});
	rg->declareLogicalStream("BloomMip3",
	                         {swapChain->hdrFormat, RGSizeMode::SixteenthExtent, vk::ImageAspectFlagBits::eColor});
	rg->declareLogicalStream("BloomMip4",
	                         {swapChain->hdrFormat, RGSizeMode::ThirtySecondExtent, vk::ImageAspectFlagBits::eColor});

	rg->setTerminalOutput("PostProcessColor", "swapChainImage"); // The final target for post-process chains
	rg->setTerminalOutput("shadowMap",
	                      "shadowMap"); // The shadow pass writes directly to the imported physical shadow map

#pragma endregion

	ShaderReloader* shaderReloader = new ShaderReloader(HALCYON_SHADER_SRC_DIR, "shaders");
	Entity shaderReloaderEntity = gm.createEntity();
	gm.registerContext<ShaderReloaderContext>(shaderReloaderEntity);
	gm.addComponent<ShaderReloaderComponent>(shaderReloaderEntity, shaderReloader);
	gm.addComponent<NameComponent>(shaderReloaderEntity, "SYSTEM Shader Reloader");

#pragma region Pipelines
	Entity pManagerEntity = gm.createEntity();
	gm.registerContext<PipelineManagerContext>(pManagerEntity);
	PipelineManager* pManager = new PipelineManager(*vulkanDevice, *dManager);
	gm.addComponent<PipelineManagerComponent>(pManagerEntity, pManager);
	gm.addComponent<NameComponent>(pManagerEntity, "SYSTEM Pipeline Manager");

	auto bindingDesc = Vertex::getBindingDescription();
	auto attrDescs = Vertex::getAttributeDescriptions();
	auto depthFormat = tManager->findBestFormat();

	std::vector<vk::Format> hdrFormats = {swapChain->hdrFormat, vk::Format::eR16G16B16A16Sfloat};

	std::vector<std::string> mainLayouts = {"globalSet", "modelSet", "textureSet"};
	// === Shadow (direct) ===
	pManager->build(PipelineDescription{
	    .shaderPath = "shaders/shadow.spv",
	    .fragEntry = "", // vertex only
	    .vertexBindings = {bindingDesc},
	    .vertexAttributes = std::vector<vk::VertexInputAttributeDescription>(attrDescs.begin(), attrDescs.end()),
	    .cullMode = vk::CullModeFlagBits::eBack,
	    .depthTest = true,
	    .depthWrite = true,
	    .depthOp = vk::CompareOp::eGreater,
	    .colorFormats = {},
	    .depthFormat = depthFormat,
	    .rasterizationSamples = vk::SampleCountFlagBits::e1,
	    .setLayoutNames = mainLayouts,
	});

	// === Depth prepass ===
	pManager->build(PipelineDescription{
	    .shaderPath = "shaders/depth_prepass.spv",
	    .fragEntry = "", // vertex only
	    .vertexBindings = {bindingDesc},
	    .vertexAttributes = std::vector<vk::VertexInputAttributeDescription>(attrDescs.begin(), attrDescs.end()),
	    .cullMode = vk::CullModeFlagBits::eBack,
	    .depthTest = true,
	    .depthWrite = true,
	    .depthOp = vk::CompareOp::eGreater,
	    .colorFormats = {},
	    .depthFormat = depthFormat,
	    .rasterizationSamples = msaaSamples,
	    .setLayoutNames = mainLayouts,
	});

	// === Graphics (opaque, IBL) ===
	pManager->build(
	    PipelineDescription{
	        .shaderPath = "shaders/standard_forward.spv",
	        .specializationValues = {0, 1}, // ALPHA_TEST=0, IBL=1
	        .vertexBindings = {bindingDesc},
	        .vertexAttributes = std::vector<vk::VertexInputAttributeDescription>(attrDescs.begin(), attrDescs.end()),
	        .cullMode = vk::CullModeFlagBits::eBack,
	        .depthTest = true,
	        .depthWrite = false,
	        .depthOp = vk::CompareOp::eEqual,
	        .colorAttachments = {PipelineFactory::blendedAttachment(), PipelineFactory::blendedAttachment()},
	        .colorFormats = hdrFormats,
	        .depthFormat = depthFormat,
	        .rasterizationSamples = msaaSamples,
	        .setLayoutNames = mainLayouts,
	    },
	    "standard_forward_opaque");

	// === Graphics (opaque, no IBL) ===
	pManager->build(
	    PipelineDescription{
	        .shaderPath = "shaders/standard_forward.spv",
	        .specializationValues = {0, 0}, // ALPHA_TEST=0, IBL=0
	        .vertexBindings = {bindingDesc},
	        .vertexAttributes = std::vector<vk::VertexInputAttributeDescription>(attrDescs.begin(), attrDescs.end()),
	        .cullMode = vk::CullModeFlagBits::eBack,
	        .depthTest = true,
	        .depthWrite = false,
	        .depthOp = vk::CompareOp::eEqual,
	        .colorAttachments = {PipelineFactory::blendedAttachment(), PipelineFactory::blendedAttachment()},
	        .colorFormats = hdrFormats,
	        .depthFormat = depthFormat,
	        .rasterizationSamples = msaaSamples,
	        .setLayoutNames = mainLayouts,
	    },
	    "standard_forward_opaque_no_ibl");

	// === Graphics (alpha test, IBL) ===
	pManager->build(
	    PipelineDescription{
	        .shaderPath = "shaders/standard_forward.spv",
	        .specializationValues = {1, 1}, // ALPHA_TEST=1, IBL=1
	        .vertexBindings = {bindingDesc},
	        .vertexAttributes = std::vector<vk::VertexInputAttributeDescription>(attrDescs.begin(), attrDescs.end()),
	        .cullMode = vk::CullModeFlagBits::eBack,
	        .depthTest = true,
	        .depthWrite = true,
	        .depthOp = vk::CompareOp::eGreater,
	        .colorAttachments = {PipelineFactory::blendedAttachment(), PipelineFactory::blendedAttachment()},
	        .colorFormats = hdrFormats,
	        .depthFormat = depthFormat,
	        .rasterizationSamples = msaaSamples,
	        .setLayoutNames = mainLayouts,
	    },
	    "standard_forward_alpha");

	// === Graphics (alpha test, no IBL) ===
	pManager->build(
	    PipelineDescription{
	        .shaderPath = "shaders/standard_forward.spv",
	        .specializationValues = {1, 0}, // ALPHA_TEST=1, IBL=0
	        .vertexBindings = {bindingDesc},
	        .vertexAttributes = std::vector<vk::VertexInputAttributeDescription>(attrDescs.begin(), attrDescs.end()),
	        .cullMode = vk::CullModeFlagBits::eBack,
	        .depthTest = true,
	        .depthWrite = true,
	        .depthOp = vk::CompareOp::eGreater,
	        .colorAttachments = {PipelineFactory::blendedAttachment(), PipelineFactory::blendedAttachment()},
	        .colorFormats = hdrFormats,
	        .depthFormat = depthFormat,
	        .rasterizationSamples = msaaSamples,
	        .setLayoutNames = mainLayouts,
	    },
	    "standard_forward_alpha_no_ibl");

	// === Capture ===
	pManager->build(
	    PipelineDescription{
	        .shaderPath = "shaders/global_illumination_forward.spv",
	        .specializationValues = {0, 1}, // ALPHA_TEST=0, IBL=1
	        .vertexBindings = {bindingDesc},
	        .vertexAttributes = std::vector<vk::VertexInputAttributeDescription>(attrDescs.begin(), attrDescs.end()),
	        .cullMode = vk::CullModeFlagBits::eBack,
	        .depthTest = true,
	        .depthWrite = true,
	        .depthOp = vk::CompareOp::eGreater,
	        .colorAttachments = {PipelineFactory::blendedAttachment()},
	        .colorFormats = {swapChain->hdrFormat},
	        .depthFormat = depthFormat,
	        .rasterizationSamples = vk::SampleCountFlagBits::e1,
	        .setLayoutNames = mainLayouts,
	    },
	    "global_illumination_forward");

	// === Capture alpha  ===
	pManager->build(
	    PipelineDescription{
	        .shaderPath = "shaders/global_illumination_forward.spv",
	        .specializationValues = {1, 1}, // ALPHA_TEST=1, IBL=1
	        .vertexBindings = {bindingDesc},
	        .vertexAttributes = std::vector<vk::VertexInputAttributeDescription>(attrDescs.begin(), attrDescs.end()),
	        .cullMode = vk::CullModeFlagBits::eBack,
	        .depthTest = true,
	        .depthWrite = true,
	        .depthOp = vk::CompareOp::eGreater,
	        .colorAttachments = {PipelineFactory::blendedAttachment()},
	        .colorFormats = {swapChain->hdrFormat},
	        .depthFormat = depthFormat,
	        .rasterizationSamples = vk::SampleCountFlagBits::e1,
	        .setLayoutNames = mainLayouts,
	    },
	    "global_illumination_forward_alpha");

	// === Skybox ===
	pManager->build(PipelineDescription{
	    .shaderPath = "shaders/skybox.spv",
	    .cullMode = vk::CullModeFlagBits::eNone,
	    .depthTest = true,
	    .depthWrite = false,
	    .depthOp = vk::CompareOp::eEqual,
	    .colorAttachments = {PipelineFactory::blendedAttachment(), PipelineFactory::blendedAttachment()},
	    .colorFormats = hdrFormats,
	    .depthFormat = depthFormat,
	    .rasterizationSamples = msaaSamples,
	    .setLayoutNames = mainLayouts,
	});

	// === Skybox for baking ===
	pManager->build(
	    PipelineDescription{
	        .shaderPath = "shaders/global_illumination_skybox.spv",
	        .cullMode = vk::CullModeFlagBits::eNone,
	        .depthTest = true,
	        .depthWrite = false,
	        .depthOp = vk::CompareOp::eEqual,
	        .colorAttachments = {PipelineFactory::blendedAttachment()},
	        .colorFormats = {swapChain->hdrFormat},
	        .depthFormat = depthFormat,
	        .rasterizationSamples = vk::SampleCountFlagBits::e1,
	        .setLayoutNames = mainLayouts,
	    },
	    "skybox_capture");

	// === Fullscreen passes ===
	pManager->build(PipelineDescription{
	    .shaderPath = "shaders/vignette.spv",
	    .cullMode = vk::CullModeFlagBits::eNone,
	    .colorAttachments = {PipelineFactory::opaqueAttachment()},
	    .colorFormats = {swapChain->swapChainImageFormat},
	    .setLayoutNames = {"screenSpaceSet"},
	});

	pManager->build(PipelineDescription{
	    .shaderPath = "shaders/tone_mapping.spv",
	    .cullMode = vk::CullModeFlagBits::eNone,
	    .colorAttachments = {PipelineFactory::opaqueAttachment()},
	    .colorFormats = {swapChain->swapChainImageFormat},
	    .setLayoutNames = {"screenSpaceSet"},
	});

	pManager->build(PipelineDescription{
	    .shaderPath = "shaders/fxaa.spv",
	    .cullMode = vk::CullModeFlagBits::eNone,
	    .colorAttachments = {PipelineFactory::opaqueAttachment()},
	    .colorFormats = {swapChain->swapChainImageFormat},
	    .setLayoutNames = {"screenSpaceSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eFragment, 0, sizeof(float) * 2}},
	});
	pManager->build(PipelineDescription{
	    .shaderPath = "shaders/ssao.spv",
	    .cullMode = vk::CullModeFlagBits::eNone,
	    .colorAttachments = {PipelineFactory::opaqueAttachment()},
	    .colorFormats = {vk::Format::eR8Unorm},
	    .setLayoutNames = {"screenSpaceSet", "globalSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eFragment, 0, 44u}},
	});
	pManager->build(PipelineDescription{
	    .shaderPath = "shaders/ssao_blur.spv",
	    .cullMode = vk::CullModeFlagBits::eNone,
	    .colorAttachments = {PipelineFactory::opaqueAttachment()},
	    .colorFormats = {vk::Format::eR8Unorm},
	    .setLayoutNames = {"screenSpaceSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eFragment, 0, sizeof(float) * 4}},
	});
	pManager->build(PipelineDescription{
	    .shaderPath = "shaders/ssao_apply.spv",
	    .cullMode = vk::CullModeFlagBits::eNone,
	    .colorAttachments = {PipelineFactory::opaqueAttachment()},
	    .colorFormats = {swapChain->hdrFormat},
	    .setLayoutNames = {"screenSpaceSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eFragment, 0, sizeof(float) * 2}},
	});

	pManager->build(PipelineDescription{
	    .shaderPath = "shaders/bloom_downsample.spv",
	    .cullMode = vk::CullModeFlagBits::eNone,
	    .colorAttachments = {PipelineFactory::opaqueAttachment()},
	    .colorFormats = {swapChain->hdrFormat},
	    .setLayoutNames = {"screenSpaceSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eFragment, 0, 20u}},
	});
	pManager->build(PipelineDescription{
	    .shaderPath = "shaders/bloom_upsample.spv",
	    .cullMode = vk::CullModeFlagBits::eNone,
	    .colorAttachments = {PipelineFactory::opaqueAttachment()},
	    .colorFormats = {swapChain->hdrFormat},
	    .setLayoutNames = {"screenSpaceSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eFragment, 0, 16u}},
	});

	// === Compute pipelines ===
	pManager->build(PipelineDescription{
	    .isCompute = true,
	    .shaderPath = "shaders/frustum_culling.spv",
	    .setLayoutNames = {"globalSet", "modelSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t)}},
	});

	pManager->build(PipelineDescription{
	    .isCompute = true,
	    .shaderPath = "shaders/shadow_frustum_culling.spv",
	    .setLayoutNames = {"globalSet", "modelSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t)}},
	});

	pManager->build(PipelineDescription{
	    .isCompute = true,
	    .shaderPath = "shaders/frustum_compaction.spv",
	    .setLayoutNames = {"modelSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t) * 4}},
	});

	pManager->build(PipelineDescription{
	    .isCompute = true,
	    .shaderPath = "shaders/reset_instance_count.spv",
	    .setLayoutNames = {"modelSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t)}},
	});

	pManager->build(PipelineDescription{
	    .isCompute = true,
	    .shaderPath = "shaders/reset_instance_count.spv",
	    .setLayoutNames = {"modelSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t)}},
	});
	pManager->build(PipelineDescription{
	    .isCompute = true,
	    .shaderPath = "shaders/equirect_to_cube.spv",
	    .setLayoutNames = {"textureSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t)}},
	});
	pManager->build(PipelineDescription{
	    .isCompute = true,
	    .shaderPath = "shaders/sh_projection.spv",
	    .setLayoutNames = {"globalSet", "textureSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(int) * 2}},
	});
	pManager->build(PipelineDescription{
	    .isCompute = true,
	    .shaderPath = "shaders/prefilter_env_map.spv",
	    .setLayoutNames = {"textureSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(float)}},
	});
	pManager->build(PipelineDescription{
	    .isCompute = true,
	    .shaderPath = "shaders/brdf_lut.spv",
	    .setLayoutNames = {"textureSet"},
	});

	// === AABB debug overlay ===
	pManager->build(
	    PipelineDescription{
	        .shaderPath = "shaders/aabb_debug.spv",
	        .topology = vk::PrimitiveTopology::eLineList,
	        .cullMode = vk::CullModeFlagBits::eNone,
	        .depthTest = true,
	        .depthWrite = false,
	        .depthOp = vk::CompareOp::eGreater,
	        .colorAttachments = {PipelineFactory::opaqueAttachment()},
	        .colorFormats = {swapChain->hdrFormat},
	        .depthFormat = depthFormat,
	        .setLayoutNames = {"globalSet"},
	        .pushConstants = {{vk::ShaderStageFlagBits::eVertex, 0, 96u}},
	    },
	    "aabb_debug");

	pManager->build(
	    PipelineDescription{
	        .shaderPath = "shaders/aabb_debug.spv",
	        .topology = vk::PrimitiveTopology::eLineList,
	        .cullMode = vk::CullModeFlagBits::eNone,
	        .depthTest = false,
	        .depthWrite = false,
	        .colorAttachments = {PipelineFactory::opaqueAttachment()},
	        .colorFormats = {swapChain->hdrFormat},
	        .depthFormat = depthFormat,
	        .setLayoutNames = {"globalSet"},
	        .pushConstants = {{vk::ShaderStageFlagBits::eVertex, 0, 96u}},
	    },
	    "aabb_debug_ontop");

	// === GI Probe debug visualization ===
	pManager->build(
	    PipelineDescription{
	        .shaderPath = "shaders/gi_probe_debug.spv",
	        .topology = vk::PrimitiveTopology::eTriangleList,
	        .cullMode = vk::CullModeFlagBits::eBack,
	        .depthTest = true,
	        .depthWrite = false,
	        .depthOp = vk::CompareOp::eGreater,
	        .colorAttachments = {PipelineFactory::opaqueAttachment()},
	        .colorFormats = {swapChain->hdrFormat},
	        .depthFormat = depthFormat,
	        .setLayoutNames = {"globalSet"},
	        .pushConstants = {{vk::ShaderStageFlagBits::eVertex, 0, 32u}}, // GIProbePush: 2×(float3+float) = 32 bytes
	    },
	    "gi_probe_debug");
#pragma endregion

#ifdef _DEBUG
	std::cout << "GraphicsPipelinesInit::VULKANNEEDS::Succes!" << std::endl;
#endif //_DEBUG
}
#pragma endregion

// Its so bad, its not even a joke anymore.
// TODO: Move it to PipelineManager.
void GraphicsPipelinesInit::recreateMsaaPipelines(GeneralManager& gm, vk::SampleCountFlagBits msaaSamples)
{
	VulkanDevice* vulkanDevice =
	    gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	auto* rg = gm.getContextComponent<RenderGraphContext, RenderGraphComponent>()->renderGraph;
	auto* tManager = gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	auto* swapChain = gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	DescriptorManager* dManager =
	    gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	PipelineManager* pManager =
	    gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;

	vulkanDevice->device.waitIdle();

	rg->declareLogicalStream(
	    "DepthMSAA", {tManager->findBestFormat(), RGSizeMode::FullExtent, vk::ImageAspectFlagBits::eDepth, msaaSamples});
	rg->declareLogicalStream(
	    "MainColorMSAA", {swapChain->hdrFormat, RGSizeMode::FullExtent, vk::ImageAspectFlagBits::eColor, msaaSamples});
	rg->declareLogicalStream("ViewNormalsMSAA", {vk::Format::eR16G16B16A16Sfloat, RGSizeMode::FullExtent,
	                                             vk::ImageAspectFlagBits::eColor, msaaSamples});

	rg->handleResize(swapChain->swapChainExtent.width, swapChain->swapChainExtent.height);

	auto bindingDesc = Vertex::getBindingDescription();
	auto attrDescs = Vertex::getAttributeDescriptions();
	auto depthFmt = tManager->findBestFormat();

	std::vector<vk::Format> hdrFormats = {swapChain->hdrFormat, vk::Format::eR16G16B16A16Sfloat};

	std::vector<std::string> mainLayouts = {"globalSet", "modelSet", "textureSet"};

	pManager->rebuild(
	    PipelineDescription{
	        .shaderPath = "shaders/depth_prepass.spv",
	        .fragEntry = "", // vertex only
	        .vertexBindings = {bindingDesc},
	        .vertexAttributes = std::vector<vk::VertexInputAttributeDescription>(attrDescs.begin(), attrDescs.end()),
	        .cullMode = vk::CullModeFlagBits::eBack,
	        .depthTest = true,
	        .depthWrite = true,
	        .depthOp = vk::CompareOp::eGreater,
	        .colorFormats = {},
	        .depthFormat = depthFmt,
	        .rasterizationSamples = msaaSamples,
	        .setLayoutNames = mainLayouts,
	    },
	    "depth_prepass");

	// === Graphics (opaque, IBL) ===
	pManager->rebuild(
	    PipelineDescription{
	        .shaderPath = "shaders/standard_forward.spv",
	        .specializationValues = {0, 1}, // ALPHA_TEST=0, IBL=1
	        .vertexBindings = {bindingDesc},
	        .vertexAttributes = std::vector<vk::VertexInputAttributeDescription>(attrDescs.begin(), attrDescs.end()),
	        .cullMode = vk::CullModeFlagBits::eBack,
	        .depthTest = true,
	        .depthWrite = false,
	        .depthOp = vk::CompareOp::eEqual,
	        .colorAttachments = {PipelineFactory::blendedAttachment(), PipelineFactory::blendedAttachment()},
	        .colorFormats = hdrFormats,
	        .depthFormat = depthFmt,
	        .rasterizationSamples = msaaSamples,
	        .setLayoutNames = mainLayouts,
	    },
	    "standard_forward_opaque");

	// === Graphics (opaque, no IBL) ===
	pManager->rebuild(
	    PipelineDescription{
	        .shaderPath = "shaders/standard_forward.spv",
	        .specializationValues = {0, 0}, // ALPHA_TEST=0, IBL=0
	        .vertexBindings = {bindingDesc},
	        .vertexAttributes = std::vector<vk::VertexInputAttributeDescription>(attrDescs.begin(), attrDescs.end()),
	        .cullMode = vk::CullModeFlagBits::eBack,
	        .depthTest = true,
	        .depthWrite = false,
	        .depthOp = vk::CompareOp::eEqual,
	        .colorAttachments = {PipelineFactory::blendedAttachment(), PipelineFactory::blendedAttachment()},
	        .colorFormats = hdrFormats,
	        .depthFormat = depthFmt,
	        .rasterizationSamples = msaaSamples,
	        .setLayoutNames = mainLayouts,
	    },
	    "standard_forward_opaque_no_ibl");

	// === Graphics (alpha test, IBL) ===
	pManager->rebuild(
	    PipelineDescription{
	        .shaderPath = "shaders/standard_forward.spv",
	        .specializationValues = {1, 1}, // ALPHA_TEST=1, IBL=1
	        .vertexBindings = {bindingDesc},
	        .vertexAttributes = std::vector<vk::VertexInputAttributeDescription>(attrDescs.begin(), attrDescs.end()),
	        .cullMode = vk::CullModeFlagBits::eBack,
	        .depthTest = true,
	        .depthWrite = true,
	        .depthOp = vk::CompareOp::eGreater,
	        .colorAttachments = {PipelineFactory::blendedAttachment(), PipelineFactory::blendedAttachment()},
	        .colorFormats = hdrFormats,
	        .depthFormat = depthFmt,
	        .rasterizationSamples = msaaSamples,
	        .setLayoutNames = mainLayouts,
	    },
	    "standard_forward_alpha");

	// === Graphics (alpha test, no IBL) ===
	pManager->rebuild(
	    PipelineDescription{
	        .shaderPath = "shaders/standard_forward.spv",
	        .specializationValues = {1, 0}, // ALPHA_TEST=1, IBL=0
	        .vertexBindings = {bindingDesc},
	        .vertexAttributes = std::vector<vk::VertexInputAttributeDescription>(attrDescs.begin(), attrDescs.end()),
	        .cullMode = vk::CullModeFlagBits::eBack,
	        .depthTest = true,
	        .depthWrite = true,
	        .depthOp = vk::CompareOp::eGreater,
	        .colorAttachments = {PipelineFactory::blendedAttachment(), PipelineFactory::blendedAttachment()},
	        .colorFormats = hdrFormats,
	        .depthFormat = depthFmt,
	        .rasterizationSamples = msaaSamples,
	        .setLayoutNames = mainLayouts,
	    },
	    "standard_forward_alpha_no_ibl");

	pManager->rebuild(
	    PipelineDescription{
	        .shaderPath = "shaders/skybox.spv",
	        .cullMode = vk::CullModeFlagBits::eNone,
	        .depthTest = true,
	        .depthWrite = false,
	        .depthOp = vk::CompareOp::eEqual,
	        .colorAttachments = {PipelineFactory::blendedAttachment(), PipelineFactory::blendedAttachment()},
	        .colorFormats = hdrFormats,
	        .depthFormat = depthFmt,
	        .rasterizationSamples = msaaSamples,
	        .setLayoutNames = mainLayouts,
	    },
	    "skybox");
}