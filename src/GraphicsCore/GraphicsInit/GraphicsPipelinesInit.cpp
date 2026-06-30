#include "GraphicsPipelinesInit.hpp"
#include "GraphicsCore/Factories/PipelineFactory.hpp"
#include <iostream>

#include "DeletionQueueComponent.hpp"
#include "DeletionQueueContext.hpp"
#include <vk_mem_alloc.h>
#include "GraphicsCore/Resources/Managers/Vertex.hpp"
#include "GraphicsCore/Components/VulkanDeviceComponent.hpp"
#include "GraphicsCore/Components/SwapChainComponent.hpp"
#include "GraphicsCore/Components/VMAllocatorComponent.hpp"
#include "GraphicsCore/Components/TextureManagerComponent.hpp"
#include "GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include "GraphicsCore/Components/RenderGraphComponent.hpp"
#include "GraphicsCore/Components/PipelineManagerComponent.hpp"
#include "GraphicsCore/Components/GraphicsSettingsComponent.hpp"
#include "GraphicsCore/RenderGraph/RenderGraph.hpp"
#include "GraphicsCore/Components/NameComponent.hpp"
#include "GraphicsCore/VulkanDevice.hpp"
#include "GraphicsCore/SwapChain.hpp"
#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "GraphicsCore/Resources/Managers/Bindings.hpp"
#include "GraphicsCore/Managers/PipelineManager.hpp"
#include "GraphicsCore/VulkanUtils.hpp"
#include "GraphicsCore/GraphicsContexts.hpp"
#ifdef HALCYON_DEV_TOOLS
#include "GraphicsCore/ShaderReloader.hpp"
#include "GraphicsCore/Components/ShaderReloaderComponent.hpp"
#endif
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>
#include <Orhescyon/Entitys/ActiveEntitySet.hpp>
#include <Orhescyon/GeneralManager.hpp>
#include "GraphicsCore/RenderGraph/RGResource.hpp"

#pragma region initPipelines
void GraphicsPipelinesInit::initPipelines(GeneralManager& gm)
{
	DeletionQueue* dq = gm.getContextComponent<DeletionQueueContext, DeletionQueueComponent>()->queue;

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

	Orhescyon::Entity rgEntity = gm.createEntity();
	RenderGraph* rg = new RenderGraph(*vulkanDevice, vmaAlloc, &gm);
	gm.registerContext<RenderGraphContext>(rgEntity);
	gm.addComponent<RenderGraphComponent>(rgEntity, rg);
	gm.addComponent<NameComponent>(rgEntity, "SYSTEM Render Graph");
	dq->push_function([rg]() { delete rg; });

#pragma endregion

#ifdef HALCYON_DEV_TOOLS
	ShaderReloader* shaderReloader = new ShaderReloader(HALCYON_SHADER_SRC_DIR, VulkanUtils::resolveShaderDir());
	Orhescyon::Entity shaderReloaderEntity = gm.createEntity();
	gm.registerContext<ShaderReloaderContext>(shaderReloaderEntity);
	gm.addComponent<ShaderReloaderComponent>(shaderReloaderEntity, shaderReloader);
	gm.addComponent<NameComponent>(shaderReloaderEntity, "SYSTEM Shader Reloader");
#endif

#pragma region Pipelines
	Orhescyon::Entity pManagerEntity = gm.createEntity();
	gm.registerContext<PipelineManagerContext>(pManagerEntity);
	PipelineManager* pManager = new PipelineManager(*vulkanDevice, *dManager);
	gm.addComponent<PipelineManagerComponent>(pManagerEntity, pManager);
	gm.addComponent<NameComponent>(pManagerEntity, "SYSTEM Pipeline Manager");
	dq->push_function([pManager]() { delete pManager; });

	auto bindingDesc = Vertex::getBindingDescription();
	auto attrDescs = Vertex::getAttributeDescriptions();
	auto depthFormat = tManager->findBestFormat();

	std::vector<std::string> mainLayouts = {"globalSet", "modelSet", "textureSet"};

	// === Capture ===
	pManager->build(
	    PipelineDescription{
	        .shaderPath = "global_illumination_forward.spv",
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
	        .shaderPath = "global_illumination_forward.spv",
	        .specializationValues = {1, 1}, // ALPHA_TEST=1, IBL=1
	        .vertexBindings = {bindingDesc},
	        .vertexAttributes = std::vector<vk::VertexInputAttributeDescription>(attrDescs.begin(), attrDescs.end()),
	        .cullMode = vk::CullModeFlagBits::eBack,
	        .depthTest = true,
	        .depthWrite = false,
	        .depthOp = vk::CompareOp::eGreaterOrEqual,
	        .colorAttachments = {PipelineFactory::blendedAttachment()},
	        .colorFormats = {swapChain->hdrFormat},
	        .depthFormat = depthFormat,
	        .rasterizationSamples = vk::SampleCountFlagBits::e1,
	        .setLayoutNames = mainLayouts,
	    },
	    "global_illumination_forward_alpha");

	// === Skybox for baking ===
	pManager->build(
	    PipelineDescription{
	        .shaderPath = "global_illumination_skybox.spv",
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

	// === Compute pipelines ===
	pManager->build(PipelineDescription{
	    .isCompute = true,
	    .shaderPath = "shadow_frustum_culling.spv",
	    .setLayoutNames = {"globalSet", "modelSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t)}},
	});

	pManager->build(PipelineDescription{
	    .isCompute = true,
	    .shaderPath = "frustum_compaction.spv",
	    .setLayoutNames = {"modelSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t) * 4}},
	});

	pManager->build(PipelineDescription{
	    .isCompute = true,
	    .shaderPath = "reset_instance_count.spv",
	    .setLayoutNames = {"modelSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t)}},
	});

	pManager->build(PipelineDescription{
	    .isCompute = true,
	    .shaderPath = "equirect_to_cube.spv",
	    .setLayoutNames = {"textureSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t)}},
	});
	pManager->build(PipelineDescription{
	    .isCompute = true,
	    .shaderPath = "sh_projection.spv",
	    .setLayoutNames = {"globalSet", "textureSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(int) * 2}},
	});
	pManager->build(PipelineDescription{
	    .isCompute = true,
	    .shaderPath = "prefilter_env_map.spv",
	    .setLayoutNames = {"textureSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(float)}},
	});
	pManager->build(PipelineDescription{
	    .isCompute = true,
	    .shaderPath = "brdf_lut.spv",
	    .setLayoutNames = {"textureSet"},
	});

	// === GI light source bake ===
	pManager->build(
	    PipelineDescription{
	        .shaderPath = "gi_light_source_bake.spv",
	        .topology = vk::PrimitiveTopology::eTriangleList,
	        .cullMode = vk::CullModeFlagBits::eBack,
	        .depthTest = true,
	        .depthWrite = true,
	        .depthOp = vk::CompareOp::eGreater,
	        .colorAttachments = {PipelineFactory::opaqueAttachment()},
	        .colorFormats = {swapChain->hdrFormat},
	        .depthFormat = depthFormat,
	        .setLayoutNames = {"globalSet"},
	        .pushConstants = {{vk::ShaderStageFlagBits::eVertex, 0, 4u}}, // float scale
	    },
	    "gi_light_source_bake");
#pragma endregion

#ifdef _DEBUG
	std::cout << "GraphicsPipelinesInit::VULKANNEEDS::Succes!" << std::endl;
#endif //_DEBUG
}
#pragma endregion
