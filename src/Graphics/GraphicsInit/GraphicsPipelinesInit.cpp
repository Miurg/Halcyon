#include "GraphicsPipelinesInit.hpp"
#include "../Factories/PipelineFactory.hpp"
#include <iostream>
#include <stdexcept>
#include "../Factories/VulkanDeviceFactory.hpp"
#include "../Factories/SwapChainFactory.hpp"

// Entities and Components
#include "../Components/VulkanDeviceComponent.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../Components/VMAllocatorComponent.hpp"
#include "../Components/TextureManagerComponent.hpp"
#include "../Components/BufferManagerComponent.hpp"
#include "../Components/ModelManagerComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Components/FrameManagerComponent.hpp"
#include "../Components/FrameDataComponent.hpp"
#include "../Components/CurrentFrameComponent.hpp"
#include "../Components/GraphicsSettingsComponent.hpp"
#include "../Components/FrameImageComponent.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"
#include "../Resources/Components/ModelDSetComponent.hpp"
#include "../Components/DrawInfoComponent.hpp"
#include "../Components/SsaoSettingsComponent.hpp"
#include "../Components/RenderGraphComponent.hpp"
#include "../Components/PipelineManagerComponent.hpp"
#include "../RenderGraph/RenderGraph.hpp"

#include "../Components/NameComponent.hpp"
#include "../Components/CameraComponent.hpp"
#include "../Components/DirectLightComponent.hpp"
#include "../Components/LocalTransformComponent.hpp"
#include "../Components/GlobalTransformComponent.hpp"
#include "../Components/SkyboxComponent.hpp"
#include "../../Platform/PlatformContexts.hpp"
#include "../../Platform/Components/WindowComponent.hpp"

// Managers and Resources
#include "../VulkanDevice.hpp"
#include "../SwapChain.hpp"
#include "../Resources/Managers/TextureManager.hpp"
#include "../Resources/Managers/BufferManager.hpp"
#include "../Resources/Managers/ModelManager.hpp"
#include "../Resources/Managers/DescriptorManager.hpp"
#include "../Managers/FrameManager.hpp"
#include "../Managers/PipelineManager.hpp"
#include "../Resources/Factories/TextureUploader.hpp"
#include "../VulkanUtils.hpp"
// Contexts
#include "../GraphicsContexts.hpp"
#include "../Resources/ResourceStructures.hpp"
#include "../Resources/Managers/Bindings.hpp"

// ImGui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#pragma region initPipelines
void GraphicsPipelinesInit::initPipelines(GeneralManager& gm)
{
#ifdef _DEBUG
	std::cout << "GraphicsPipelinesInit::VULKANNEEDS::Start..." << std::endl;
#endif //_DEBUG
	VulkanDevice* vulkanDevice =
	    gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	Window* window = gm.getContextComponent<MainWindowContext, WindowComponent>()->windowInstance;
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
#pragma endregion

#pragma region Pipelines

	Entity pManagerEntity = gm.createEntity();
	gm.registerContext<PipelineManagerContext>(pManagerEntity);
	PipelineManager* pManager = new PipelineManager(*vulkanDevice);
	gm.addComponent<PipelineManagerComponent>(pManagerEntity, pManager);
	gm.addComponent<NameComponent>(pManagerEntity, "SYSTEM Pipeline Manager");

	auto& dev = vulkanDevice->device;

	auto bindingDesc = Vertex::getBindingDescription();
	auto attrDescs = Vertex::getAttributeDescriptions();
	auto depthFmt = tManager->findBestFormat();

	std::vector<vk::Format> hdrFormats = {swapChain->hdrFormat, vk::Format::eR16G16B16A16Sfloat};

	std::vector<vk::DescriptorSetLayout> mainLayouts = {*dManager->globalSetLayout, *dManager->modelSetLayout,
	                                                    *dManager->textureSetLayout};
	// === Shadow (direct) ===
	pManager->build(vulkanDevice->device, PipelineDescription{
	                                          .shaderPath = "shaders/shadow.spv",
	                                          .fragEntry = "", // vertex only
	                                          .vertexBinding = &bindingDesc,
	                                          .vertexAttributes = attrDescs.data(),
	                                          .attributeCount = static_cast<uint32_t>(attrDescs.size()),
	                                          .cullMode = vk::CullModeFlagBits::eBack,
	                                          .depthTest = true,
	                                          .depthWrite = true,
	                                          .depthOp = vk::CompareOp::eGreater,
	                                          .colorFormats = {},
	                                          .depthFormat = depthFmt,
	                                          .rasterizationSamples = vk::SampleCountFlagBits::e1,
	                                          .setLayouts = mainLayouts,
	                                      });

	// === Depth prepass ===
	pManager->build(vulkanDevice->device, PipelineDescription{
	                                          .shaderPath = "shaders/depth_prepass.spv",
	                                          .fragEntry = "", // vertex only
	                                          .vertexBinding = &bindingDesc,
	                                          .vertexAttributes = attrDescs.data(),
	                                          .attributeCount = static_cast<uint32_t>(attrDescs.size()),
	                                          .cullMode = vk::CullModeFlagBits::eBack,
	                                          .depthTest = true,
	                                          .depthWrite = true,
	                                          .depthOp = vk::CompareOp::eGreater,
	                                          .colorFormats = {},
	                                          .depthFormat = depthFmt,
	                                          .rasterizationSamples = msaaSamples,
	                                          .setLayouts = mainLayouts,
	                                      });

	// === Graphics (opaque) ===
	pManager->build(dev,
	                PipelineDescription{
	                    .shaderPath = "shaders/standard_forward.spv",
	                    .specializationValue = 0,
	                    .vertexBinding = &bindingDesc,
	                    .vertexAttributes = attrDescs.data(),
	                    .attributeCount = static_cast<uint32_t>(attrDescs.size()),
	                    .cullMode = vk::CullModeFlagBits::eBack,
	                    .depthTest = true,
	                    .depthWrite = false,
	                    .depthOp = vk::CompareOp::eEqual,
	                    .colorAttachments = {PipelineFactory::blendedAttachment(), PipelineFactory::blendedAttachment()},
	                    .colorFormats = hdrFormats,
	                    .depthFormat = depthFmt,
	                    .rasterizationSamples = msaaSamples,
	                    .setLayouts = mainLayouts,
	                },
	                "standard_forward_opaque");

	// === Graphics (alpha test) ===
	pManager->build(dev,
	                PipelineDescription{
	                    .shaderPath = "shaders/standard_forward.spv",
	                    .specializationValue = 1,
	                    .vertexBinding = &bindingDesc,
	                    .vertexAttributes = attrDescs.data(),
	                    .attributeCount = static_cast<uint32_t>(attrDescs.size()),
	                    .cullMode = vk::CullModeFlagBits::eBack,
	                    .depthTest = true,
	                    .depthWrite = true,
	                    .depthOp = vk::CompareOp::eGreater,
	                    .colorAttachments = {PipelineFactory::blendedAttachment(), PipelineFactory::blendedAttachment()},
	                    .colorFormats = hdrFormats,
	                    .depthFormat = depthFmt,
	                    .rasterizationSamples = msaaSamples,
	                    .setLayouts = mainLayouts,
	                },
	                "standard_forward_alpha");

	// === Skybox ===
	pManager->build(dev,
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
	                    .setLayouts = mainLayouts,
	                });

	// === Fullscreen passes ===
	pManager->build(dev, PipelineDescription{
	                         .shaderPath = "shaders/tone_mapping.spv",
	                         .cullMode = vk::CullModeFlagBits::eNone,
	                         .colorAttachments = {PipelineFactory::opaqueAttachment()},
	                         .colorFormats = {swapChain->swapChainImageFormat},
	                         .setLayouts = {*dManager->screenSpaceSetLayout},
	                     });

	pManager->build(dev, PipelineDescription{
	                         .shaderPath = "shaders/fxaa.spv",
	                         .cullMode = vk::CullModeFlagBits::eNone,
	                         .colorAttachments = {PipelineFactory::opaqueAttachment()},
	                         .colorFormats = {swapChain->swapChainImageFormat},
	                         .setLayouts = {*dManager->screenSpaceSetLayout},
	                         .pushConstants = {{vk::ShaderStageFlagBits::eFragment, 0, sizeof(float) * 2}},
	                     });
	pManager->build(dev, PipelineDescription{
	                         .shaderPath = "shaders/ssao.spv",
	                         .cullMode = vk::CullModeFlagBits::eNone,
	                         .colorAttachments = {PipelineFactory::opaqueAttachment()},
	                         .colorFormats = {vk::Format::eR8Unorm},
	                         .setLayouts = {*dManager->screenSpaceSetLayout, *dManager->globalSetLayout},
	                         .pushConstants = {{vk::ShaderStageFlagBits::eFragment, 0, 40u}},
	                     });
	pManager->build(dev, PipelineDescription{
	                         .shaderPath = "shaders/ssao_blur.spv",
	                         .cullMode = vk::CullModeFlagBits::eNone,
	                         .colorAttachments = {PipelineFactory::opaqueAttachment()},
	                         .colorFormats = {vk::Format::eR8Unorm},
	                         .setLayouts = {*dManager->screenSpaceSetLayout},
	                         .pushConstants = {{vk::ShaderStageFlagBits::eFragment, 0, sizeof(float) * 2}},
	                     });
	pManager->build(dev, PipelineDescription{
	                         .shaderPath = "shaders/ssao_apply.spv",
	                         .cullMode = vk::CullModeFlagBits::eNone,
	                         .colorAttachments = {PipelineFactory::opaqueAttachment()},
	                         .colorFormats = {swapChain->hdrFormat},
	                         .setLayouts = {*dManager->screenSpaceSetLayout},
	                         .pushConstants = {{vk::ShaderStageFlagBits::eFragment, 0, sizeof(float) * 2}},
	                     });

	pManager->build(dev, PipelineDescription{
	                         .shaderPath = "shaders/bloom_downsample.spv",
	                         .cullMode = vk::CullModeFlagBits::eNone,
	                         .colorAttachments = {PipelineFactory::opaqueAttachment()},
	                         .colorFormats = {swapChain->hdrFormat},
	                         .setLayouts = {*dManager->screenSpaceSetLayout},
	                         .pushConstants = {{vk::ShaderStageFlagBits::eFragment, 0, 20u}},
	                     });
	pManager->build(dev, PipelineDescription{
	                         .shaderPath = "shaders/bloom_upsample.spv",
	                         .cullMode = vk::CullModeFlagBits::eNone,
	                         .colorAttachments = {PipelineFactory::opaqueAttachment()},
	                         .colorFormats = {swapChain->hdrFormat},
	                         .setLayouts = {*dManager->screenSpaceSetLayout},
	                         .pushConstants = {{vk::ShaderStageFlagBits::eFragment, 0, 16u}},
	                     });

	// === Compute pipelines ===
	pManager->build(dev, PipelineDescription{
	                         .isCompute = true,
	                         .shaderPath = "shaders/frustum_culling.spv",
	                         .setLayouts = {*dManager->globalSetLayout, *dManager->modelSetLayout},
	                         .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t)}},
	                     });

	pManager->build(dev, PipelineDescription{
	                         .isCompute = true,
	                         .shaderPath = "shaders/shadow_frustum_culling.spv",
	                         .setLayouts = {*dManager->globalSetLayout, *dManager->modelSetLayout},
	                         .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t)}},
	                     });


	pManager->build(dev, PipelineDescription{
	                         .isCompute = true,
	                         .shaderPath = "shaders/frustum_compaction.spv",
	                         .setLayouts = {*dManager->modelSetLayout},
	                         .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t) * 4}},
	                     });

	pManager->build(dev, PipelineDescription{
	                         .isCompute = true,
	                         .shaderPath = "shaders/reset_instance_count.spv",
	                         .setLayouts = {*dManager->modelSetLayout},
	                         .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t)}},
	                     });

	pManager->build(dev, PipelineDescription{
	                         .isCompute = true,
	                         .shaderPath = "shaders/reset_instance_count.spv",
	                         .setLayouts = {*dManager->modelSetLayout},
	                         .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t)}},
	                     });
	pManager->build(dev, PipelineDescription{
	                         .isCompute = true,
	                         .shaderPath = "shaders/equirect_to_cube.spv",
	                         .setLayouts = {*dManager->textureSetLayout},
	                         .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t)}},
	                     });
	pManager->build(dev, PipelineDescription{
	                         .isCompute = true,
	                         .shaderPath = "shaders/irradiance_convolution.spv",
	                         .setLayouts = {*dManager->textureSetLayout},
	                         .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(float)}},
	                     });
	pManager->build(dev, PipelineDescription{
	                         .isCompute = true,
	                         .shaderPath = "shaders/prefilter_env_map.spv",
	                         .setLayouts = {*dManager->textureSetLayout},
	                         .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(float)}},
	                     });
	pManager->build(dev, PipelineDescription{
	                         .isCompute = true,
	                         .shaderPath = "shaders/brdf_lut.spv",
	                         .setLayouts = {*dManager->textureSetLayout},
	                     });
#pragma endregion

#ifdef _DEBUG
	std::cout << "GraphicsPipelinesInit::VULKANNEEDS::Succes!" << std::endl;
#endif //_DEBUG
}
#pragma endregion

// Its so bad, its not even a joke anymore.
// TODO: Refactor this entire function to be more modular and less copy-pastey.
// Move pipeline creation to its own class or something.
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

	std::vector<vk::DescriptorSetLayout> mainLayouts = {*dManager->globalSetLayout, *dManager->modelSetLayout,
	                                                    *dManager->textureSetLayout};

	pManager->rebuild(vulkanDevice->device,
	                  PipelineDescription{
	                      .shaderPath = "shaders/depth_prepass.spv",
	                      .fragEntry = "", // vertex only
	                      .vertexBinding = &bindingDesc,
	                      .vertexAttributes = attrDescs.data(),
	                      .attributeCount = static_cast<uint32_t>(attrDescs.size()),
	                      .cullMode = vk::CullModeFlagBits::eBack,
	                      .depthTest = true,
	                      .depthWrite = true,
	                      .depthOp = vk::CompareOp::eGreater,
	                      .colorFormats = {},
	                      .depthFormat = depthFmt,
	                      .rasterizationSamples = msaaSamples,
	                      .setLayouts = mainLayouts,
	                  },
	                  "depth_prepass");

	// === Graphics (opaque) ===
	pManager->rebuild(
	    vulkanDevice->device,
	    PipelineDescription{
	        .shaderPath = "shaders/standard_forward.spv",
	        .specializationValue = 0,
	        .vertexBinding = &bindingDesc,
	        .vertexAttributes = attrDescs.data(),
	        .attributeCount = static_cast<uint32_t>(attrDescs.size()),
	        .cullMode = vk::CullModeFlagBits::eBack,
	        .depthTest = true,
	        .depthWrite = false,
	        .depthOp = vk::CompareOp::eEqual,
	        .colorAttachments = {PipelineFactory::blendedAttachment(), PipelineFactory::blendedAttachment()},
	        .colorFormats = hdrFormats,
	        .depthFormat = depthFmt,
	        .rasterizationSamples = msaaSamples,
	        .setLayouts = mainLayouts,
	    },
	    "standard_forward_opaque");

	// === Graphics (alpha test) ===
	pManager->rebuild(
	    vulkanDevice->device,
	    PipelineDescription{
	        .shaderPath = "shaders/standard_forward.spv",
	        .specializationValue = 1,
	        .vertexBinding = &bindingDesc,
	        .vertexAttributes = attrDescs.data(),
	        .attributeCount = static_cast<uint32_t>(attrDescs.size()),
	        .cullMode = vk::CullModeFlagBits::eBack,
	        .depthTest = true,
	        .depthWrite = true,
	        .depthOp = vk::CompareOp::eGreater,
	        .colorAttachments = {PipelineFactory::blendedAttachment(), PipelineFactory::blendedAttachment()},
	        .colorFormats = hdrFormats,
	        .depthFormat = depthFmt,
	        .rasterizationSamples = msaaSamples,
	        .setLayouts = mainLayouts,
	    },
	    "standard_forward_alpha");

	pManager->rebuild(
	    vulkanDevice->device,
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
	        .setLayouts = mainLayouts,
	    },
	    "skybox");
}