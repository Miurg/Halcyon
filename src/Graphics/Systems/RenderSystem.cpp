#include "RenderSystem.hpp"
#include <iostream>
#include "../GraphicsContexts.hpp"
#include <imgui.h>
#include "../Components/SwapChainComponent.hpp"
#include "../Components/BufferManagerComponent.hpp"
#include "../FrameData.hpp"
#include "../Components/FrameDataComponent.hpp"
#include "../Components/CurrentFrameComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Components/FrameImageComponent.hpp"
#include "../Components/TextureManagerComponent.hpp"
#include "../Resources/Managers/TextureManager.hpp"
#include "../Components/ModelManagerComponent.hpp"
#include "../Managers/FrameManager.hpp"
#include "../Components/FrameManagerComponent.hpp"
#include "../Components/DrawInfoComponent.hpp"
#include "../Components/SsaoSettingsComponent.hpp"
#include "../Components/RenderGraphComponent.hpp"
#include "../RenderGraph/RenderGraph.hpp"
#include "../Components/GraphicsSettingsComponent.hpp"
#include "../Resources/Managers/Bindings.hpp"
#include "../GraphicsInit/GraphicsPipelinesInit.hpp"
#include "../Managers/PipelineManager.hpp"
#include "../Components/PipelineManagerComponent.hpp"
#include "../Components/SkyboxComponent.hpp"
#include "../Resources/Components/MeshInfoComponent.hpp"
#include "../Components/RelationshipComponent.hpp"
#include "../Components/GlobalTransformComponent.hpp"
#include <functional>
#include "../Passes/RenderPasses.hpp"
#include <imgui_impl_vulkan.h>
#include "../Components/LightProbeGridComponent.hpp"
#include "../GIBaker/LightProbeGIBaking.hpp"

void RenderSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "RenderSystem registered!" << std::endl;
}

void RenderSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "RenderSystem shutdown!" << std::endl;
}

void RenderSystem::update(GeneralManager& gm)
{
	SwapChain& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	BufferManager& bufferManager =
	    *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	TextureManager& textureManager =
	    *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	ModelManager& modelManager = *gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;
	CurrentFrameComponent& currentFrameComp = *gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>();
	DirectLightComponent& lightTexture = *gm.getContextComponent<SunContext, DirectLightComponent>();
	BindlessTextureDSetComponent& materialDSetComponent =
	    *gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
	DescriptorManagerComponent& dManager =
	    *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	GlobalDSetComponent& globalDSetComponent = *gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	ModelDSetComponent& objectDSetComponent = *gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();
	DrawInfoComponent& drawInfo = *gm.getContextComponent<CurrentFrameContext, DrawInfoComponent>();
	SkyboxComponent& skyboxComp = *gm.getContextComponent<SkyBoxContext, SkyboxComponent>();
	bool hasSkybox = skyboxComp.hasSkybox;
	SsaoSettingsComponent& ssaoSettings = *gm.getContextComponent<SsaoSettingsContext, SsaoSettingsComponent>();
	RenderGraph& rg = *gm.getContextComponent<RenderGraphContext, RenderGraphComponent>()->renderGraph;
	GraphicsSettingsComponent& graphicsSettings =
	    *gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>();
	uint32_t imageIndex = gm.getContextComponent<FrameImageContext, FrameImageComponent>()->imageIndex;
	PipelineManager& pManager =
	    *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	LightProbeGridComponent* probesGrid = gm.getContextComponent<LightProbeGridContext, LightProbeGridComponent>();

	if (!currentFrameComp.frameValid) return;

	// Rebake grid if need.
	if (probesGrid != nullptr && probesGrid->needBake)
	{
		LightProbeGIBaking::bakeAll(gm);
		probesGrid->needBake = false;
	}

	ImGui::Render();

	uint32_t currentFrame = currentFrameComp.currentFrame;

	// Kainda static resources - imported each frame but same handles
	auto shadowMapHandle = rg.importImage(
	    "shadowMap", textureManager.textures[lightTexture.textureShadowImage.id].textureImage,
	    textureManager.textures[lightTexture.textureShadowImage.id].textureImageView, vk::ImageAspectFlagBits::eDepth);
	auto swapChainHandle = rg.importImage("swapChainImage", swapChain.swapChainImages[imageIndex],
	                                      swapChain.swapChainImageViews[imageIndex], vk::ImageAspectFlagBits::eColor);
	auto noiseHandle =
	    rg.importImage("NoiseImage", textureManager.textures[swapChain.ssaoNoiseTextureHandle.id].textureImage,
	                   textureManager.textures[swapChain.ssaoNoiseTextureHandle.id].textureImageView,
	                   vk::ImageAspectFlagBits::eColor, vk::ImageLayout::eShaderReadOnlyOptimal);

	if (graphicsSettings.msaaSamples != graphicsSettings.appliedMsaaSamples)
	{
		GraphicsPipelinesInit::recreateMsaaPipelines(gm, graphicsSettings.msaaSamples);
		graphicsSettings.appliedMsaaSamples = graphicsSettings.msaaSamples;
	}

	RenderPasses::DirectLightPass(currentFrame, dManager, globalDSetComponent, bufferManager, objectDSetComponent,
	                              modelManager, drawInfo, pManager, rg, textureManager, lightTexture);

	RenderPasses::CullPass(currentFrame, dManager, globalDSetComponent, bufferManager, objectDSetComponent, modelManager,
	                       drawInfo, pManager, rg);

	RenderPasses::DepthPrepass(swapChain, currentFrame, materialDSetComponent, dManager, globalDSetComponent,
	                           bufferManager, objectDSetComponent, modelManager, drawInfo, pManager, graphicsSettings,
	                           rg);
	RenderPasses::MainPass(swapChain, currentFrame, materialDSetComponent, dManager, globalDSetComponent, bufferManager,
	                       objectDSetComponent, modelManager, drawInfo, pManager, hasSkybox, graphicsSettings, rg);
	RenderPasses::DebugPass(swapChain, dManager, globalDSetComponent, pManager, currentFrame, rg, gm, graphicsSettings,
	                        modelManager);

	if (graphicsSettings.enableSsao)
	{
		RenderPasses::SSAOPass(swapChain, currentFrameComp, materialDSetComponent, dManager, globalDSetComponent,
		                       pManager, rg, ssaoSettings);
	}

	if (graphicsSettings.enableBloom)
	{
		RenderPasses::BloomPass(swapChain, dManager, globalDSetComponent, pManager, rg, graphicsSettings);
	}

	RenderPasses::ToneMappingPass(swapChain, dManager, globalDSetComponent, pManager, rg);

	if (graphicsSettings.enableFxaa)
	{
		RenderPasses::FXAAPass(swapChain, dManager, globalDSetComponent, pManager, rg);
	}

	if (graphicsSettings.enableVignette)
	{
		RenderPasses::VignettePass(swapChain, dManager, globalDSetComponent, pManager, rg);
	}

	rg.addPass("ImGui",
	           {.colorAttachments = {{"PostProcessColor", vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore}}},
	           {}, // Keep it empty, we dont read PostProcessColor in the shader, we just bind it as a render target. The
	               // ImGui writes to it directly.
	           {{"PostProcessColor", RGResourceUsage::ColorAttachmentWrite}},
	           [&](vk::raii::CommandBuffer& cmd)
	           {
		           ImDrawData* draw_data = ImGui::GetDrawData();
		           if (draw_data)
		           {
			           ImGui_ImplVulkan_RenderDrawData(draw_data, *cmd);
		           }
	           });

	// Present barrier
	rg.addPass("Present", {}, {{"swapChainImage", RGResourceUsage::Present}}, {}, nullptr);
}