#include "RenderSystem.hpp"
#include <iostream>
#include "../GraphicsContexts.hpp"
#include <imgui.h>
#include "../Factories/CommandBufferFactory.hpp"
#include "../Components/PipelineHandlerComponent.hpp"
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
	PipelineHandler& pipelineHandler =
	    *gm.getContextComponent<MainSignatureContext, PipelineHandlerComponent>()->pipelineHandler;
	BufferManager& bufferManager =
	    *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	TextureManager& textureManager =
	    *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	ModelManager& modelManager = *gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;
	FrameManager* frameManager = gm.getContextComponent<FrameManagerContext, FrameManagerComponent>()->frameManager;
	CurrentFrameComponent* currentFrameComp = gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>();
	LightComponent* lightTexture = gm.getContextComponent<SunContext, LightComponent>();
	BindlessTextureDSetComponent* materialDSetComponent =
	    gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
	DescriptorManagerComponent* dManager =
	    gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	GlobalDSetComponent* globalDSetComponent = gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	ModelDSetComponent* objectDSetComponent = gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();
	DrawInfoComponent* drawInfo = gm.getContextComponent<CurrentFrameContext, DrawInfoComponent>();
	SsaoSettingsComponent* ssaoSettings = gm.getContextComponent<SsaoSettingsContext, SsaoSettingsComponent>();
	RenderGraph& rg = *gm.getContextComponent<RenderGraphContext, RenderGraphComponent>()->renderGraph;
	uint32_t imageIndex = gm.getContextComponent<FrameImageContext, FrameImageComponent>()->imageIndex;
	if (!currentFrameComp->frameValid) return;

	ImGui::Render();

	uint32_t currentFrame = currentFrameComp->currentFrame;
	auto& frame = frameManager->frames[currentFrame];

	rg.clearFrame();

	// Kainda static resources - imported each frame but same handles
	auto shadowMapHandle =
	    rg.importImage("shadowMap", textureManager.textures[lightTexture->textureShadowImage.id].textureImage,
	                   vk::ImageAspectFlagBits::eDepth);
	auto swapChainHandle =
	    rg.importImage("swapChainImage", swapChain.swapChainImages[imageIndex], vk::ImageAspectFlagBits::eColor);

	// Transient resources
	auto depthHandle = rg.getHandle("depth");
	auto offscreenHandle = rg.getHandle("offscreen");
	auto viewNormalsHandle = rg.getHandle("viewNormals");
	auto ssaoHandle = rg.getHandle("ssao");
	auto ssaoBlurHandle = rg.getHandle("ssaoBlur");

	vk::ImageView depthImageView = rg.getImageView(depthHandle);
	vk::ImageView offscreenImageView = rg.getImageView(offscreenHandle);
	vk::ImageView viewNormalsImageView = rg.getImageView(viewNormalsHandle);
	vk::ImageView ssaoImageView = rg.getImageView(ssaoHandle);
	vk::ImageView ssaoBlurImageView = rg.getImageView(ssaoBlurHandle);

	rg.addPass("Shadow", /*reads=*/{},
	           /*writes=*/{{shadowMapHandle, RGResourceUsage::DepthAttachmentWrite}},
	           [&](vk::raii::CommandBuffer& primaryCmd)
	           {
		           CommandBufferFactory::recordShadowCommandBuffer(
		               frame.secondaryCommandBuffers[0], pipelineHandler, currentFrame, *lightTexture, *dManager,
		               globalDSetComponent, objectDSetComponent, textureManager, modelManager);
		           primaryCmd.executeCommands(*frame.secondaryCommandBuffers[0]);
	           });

	rg.addPass("Cull", /*reads=*/{}, /*writes=*/{},
	           [&](vk::raii::CommandBuffer& primaryCmd)
	           {
		           CommandBufferFactory::recordCullCommandBuffer(frame.secondaryCommandBuffers[1], pipelineHandler,
		                                                         currentFrame, *dManager, globalDSetComponent,
		                                                         objectDSetComponent, modelManager, *drawInfo);
		           primaryCmd.executeCommands(*frame.secondaryCommandBuffers[1]);
	           });

	rg.addPass("DepthPrepass", /*reads=*/{},
	           /*writes=*/{{depthHandle, RGResourceUsage::DepthAttachmentWrite}},
	           [&](vk::raii::CommandBuffer& primaryCmd)
	           {
		           CommandBufferFactory::recordDepthPrepassCommandBuffer(
		               frame.secondaryCommandBuffers[2], imageIndex, swapChain, pipelineHandler, currentFrame,
		               *materialDSetComponent, *dManager, globalDSetComponent, bufferManager, objectDSetComponent,
		               modelManager, depthImageView, *drawInfo);
		           primaryCmd.executeCommands(*frame.secondaryCommandBuffers[2]);
	           });

	rg.addPass("Main",
	           /*reads=*/{{shadowMapHandle, RGResourceUsage::ShaderRead}},
	           /*writes=*/
	           {{offscreenHandle, RGResourceUsage::ColorAttachmentWrite},
	            {viewNormalsHandle, RGResourceUsage::ColorAttachmentWrite},
	            {depthHandle, RGResourceUsage::DepthAttachmentWrite}},
	           [&](vk::raii::CommandBuffer& primaryCmd)
	           {
		           CommandBufferFactory::recordMainCommandBuffer(
		               frame.secondaryCommandBuffers[3], imageIndex, swapChain, pipelineHandler, currentFrame,
		               *materialDSetComponent, *dManager, globalDSetComponent, bufferManager, objectDSetComponent,
		               modelManager, offscreenImageView, viewNormalsImageView, depthImageView, *drawInfo);
		           primaryCmd.executeCommands(*frame.secondaryCommandBuffers[3]);
	           });

	rg.addPass("SSAO",
	           /*reads=*/
	           {{depthHandle, RGResourceUsage::ShaderRead}, {viewNormalsHandle, RGResourceUsage::ShaderRead}},
	           /*writes=*/{{ssaoHandle, RGResourceUsage::ColorAttachmentWrite}},
	           [&](vk::raii::CommandBuffer& primaryCmd)
	           {
		           CommandBufferFactory::recordSsaoCommandBuffer(
		               frame.secondaryCommandBuffers[4], swapChain, pipelineHandler, *dManager,
		               globalDSetComponent->ssaoDSets, globalDSetComponent->globalDSets, ssaoImageView, *ssaoSettings);
		           primaryCmd.executeCommands(*frame.secondaryCommandBuffers[4]);
	           });

	rg.addPass("SSAOBlur",
	           /*reads=*/{{ssaoHandle, RGResourceUsage::ShaderRead}},
	           /*writes=*/{{ssaoBlurHandle, RGResourceUsage::ColorAttachmentWrite}},
	           [&](vk::raii::CommandBuffer& primaryCmd)
	           {
		           CommandBufferFactory::recordSsaoBlurCommandBuffer(
		               frame.secondaryCommandBuffers[5], swapChain, pipelineHandler, *dManager,
		               globalDSetComponent->ssaoBlurDSets, ssaoBlurImageView);
		           primaryCmd.executeCommands(*frame.secondaryCommandBuffers[5]);
	           });

	rg.addPass("FXAA",
	           /*reads=*/
	           {{offscreenHandle, RGResourceUsage::ShaderRead}, {ssaoBlurHandle, RGResourceUsage::ShaderRead}},
	           /*writes=*/{{swapChainHandle, RGResourceUsage::ColorAttachmentWrite}},
	           [&](vk::raii::CommandBuffer& primaryCmd)
	           {
		           CommandBufferFactory::recordFxaaCommandBuffer(frame.secondaryCommandBuffers[6], imageIndex, swapChain,
		                                                         pipelineHandler, *dManager,
		                                                         globalDSetComponent->fxaaDSets);
		           primaryCmd.executeCommands(*frame.secondaryCommandBuffers[6]);
	           });

	// Present barrier
	rg.addPass("Present", /*reads=*/{{swapChainHandle, RGResourceUsage::Present}}, /*writes=*/{}, nullptr);

	rg.compile();

	frame.commandBuffer.begin({});
	rg.execute(frame.commandBuffer);
	frame.commandBuffer.end();
}