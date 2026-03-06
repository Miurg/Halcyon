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
	auto shadowMapHandle = rg.importImage(
	    "shadowMap", textureManager.textures[lightTexture->textureShadowImage.id].textureImage,
	    textureManager.textures[lightTexture->textureShadowImage.id].textureImageView, vk::ImageAspectFlagBits::eDepth);
	auto swapChainHandle = rg.importImage("swapChainImage", swapChain.swapChainImages[imageIndex],
	                                      swapChain.swapChainImageViews[imageIndex], vk::ImageAspectFlagBits::eColor);

	// Transient resources
	auto depthHandle = rg.getHandle("depth");
	auto offscreenHandle = rg.getHandle("offscreen");
	auto viewNormalsHandle = rg.getHandle("viewNormals");
	auto ssaoHandle = rg.getHandle("ssao");
	auto ssaoBlurHandle = rg.getHandle("ssaoBlur");

	vk::ClearValue clearSky = vk::ClearColorValue(0.0f, 0.637f, 1.0f, 1.0f);
	vk::ClearValue clearBlack = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);
	vk::ClearValue clearWhite = vk::ClearColorValue(1.0f, 1.0f, 1.0f, 1.0f);
	vk::ClearValue clearDepth1 = vk::ClearDepthStencilValue(1.0f, 0);
	vk::ClearValue clearDepth0 = vk::ClearDepthStencilValue(0.0f, 0);

	rg.addPass("ShadowCull", {.isCompute = true}, {}, {},
	           [&](vk::raii::CommandBuffer& cmd)
	           {
		           CommandBufferFactory::drawShadowCullPass(cmd, pipelineHandler, currentFrame, *dManager,
		                                                    globalDSetComponent, objectDSetComponent, modelManager,
		                                                    bufferManager, *drawInfo);
	           });

	rg.addPass("Shadow",
	           {.depthAttachment = RGAttachmentConfig{shadowMapHandle, vk::AttachmentLoadOp::eClear,
	                                                  vk::AttachmentStoreOp::eStore, clearDepth0},
	            .customExtent = vk::Extent2D{static_cast<uint32_t>(lightTexture->sizeX),
	                                         static_cast<uint32_t>(lightTexture->sizeY)}},
	           {}, {{shadowMapHandle, RGResourceUsage::DepthAttachmentWrite}},
	           [&](vk::raii::CommandBuffer& cmd)
	           {
		           CommandBufferFactory::drawShadowPass(cmd, pipelineHandler, currentFrame, *lightTexture, *dManager,
		                                                globalDSetComponent, objectDSetComponent, textureManager,
		                                                modelManager, bufferManager, *drawInfo);
	           });

	rg.addPass("ResetInstanceCount", {.isCompute = true}, {}, {},
	           [&](vk::raii::CommandBuffer& cmd)
	           {
		           CommandBufferFactory::drawResetInstancePass(cmd, pipelineHandler, currentFrame, *dManager,
		                                                       objectDSetComponent, *drawInfo);
	           });

	rg.addPass("Cull", {.isCompute = true}, {}, {},
	           [&](vk::raii::CommandBuffer& cmd)
	           {
		           CommandBufferFactory::drawCullPass(cmd, pipelineHandler, currentFrame, *dManager, globalDSetComponent,
		                                              objectDSetComponent, modelManager, bufferManager, *drawInfo);
	           });

	rg.addPass("DepthPrepass",
	           {.depthAttachment = RGAttachmentConfig{depthHandle, vk::AttachmentLoadOp::eClear,
	                                                  vk::AttachmentStoreOp::eStore, clearDepth0}},
	           {}, {{depthHandle, RGResourceUsage::DepthAttachmentWrite}},
	           [&](vk::raii::CommandBuffer& cmd)
	           {
		           CommandBufferFactory::drawDepthPrepass(cmd, swapChain, pipelineHandler, currentFrame,
		                                                  *materialDSetComponent, *dManager, globalDSetComponent,
		                                                  bufferManager, objectDSetComponent, modelManager, *drawInfo);
	           });

	rg.addPass(
	    "Main",
	    {.colorAttachments = {{offscreenHandle, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearSky},
	                          {viewNormalsHandle, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
	                           clearBlack}},
	     .depthAttachment =
	         RGAttachmentConfig{depthHandle, vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, clearDepth1}},
	    {{shadowMapHandle, RGResourceUsage::ShaderRead}},
	    {{offscreenHandle, RGResourceUsage::ColorAttachmentWrite},
	     {viewNormalsHandle, RGResourceUsage::ColorAttachmentWrite},
	     {depthHandle, RGResourceUsage::DepthAttachmentWrite}},
	    [&](vk::raii::CommandBuffer& cmd)
	    {
		    CommandBufferFactory::drawMainPass(cmd, swapChain, pipelineHandler, currentFrame, *materialDSetComponent,
		                                       *dManager, globalDSetComponent, bufferManager, objectDSetComponent,
		                                       modelManager, *drawInfo);
	    });

	rg.addPass(
	    "SSAO",
	    {.colorAttachments = {{ssaoHandle, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearWhite}}},
	    {{depthHandle, RGResourceUsage::ShaderRead}, {viewNormalsHandle, RGResourceUsage::ShaderRead}},
	    {{ssaoHandle, RGResourceUsage::ColorAttachmentWrite}},
	    [&](vk::raii::CommandBuffer& cmd)
	    {
		    CommandBufferFactory::drawSsaoPass(cmd, swapChain, pipelineHandler, *dManager, globalDSetComponent->ssaoDSets,
		                                       globalDSetComponent->globalDSets, *ssaoSettings);
	    });

	rg.addPass("SSAOBlur",
	           {.colorAttachments = {{ssaoBlurHandle, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
	                                  clearWhite}}},
	           {{ssaoHandle, RGResourceUsage::ShaderRead}}, {{ssaoBlurHandle, RGResourceUsage::ColorAttachmentWrite}},
	           [&](vk::raii::CommandBuffer& cmd)
	           {
		           CommandBufferFactory::drawSsaoBlurPass(cmd, swapChain, pipelineHandler, *dManager,
		                                                  globalDSetComponent->ssaoBlurDSets);
	           });

	rg.addPass("FXAA",
	           {.colorAttachments = {{swapChainHandle, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
	                                  clearBlack}}},
	           {{offscreenHandle, RGResourceUsage::ShaderRead}, {ssaoBlurHandle, RGResourceUsage::ShaderRead}},
	           {{swapChainHandle, RGResourceUsage::ColorAttachmentWrite}},
	           [&](vk::raii::CommandBuffer& cmd)
	           {
		           CommandBufferFactory::drawFxaaPass(cmd, swapChain, pipelineHandler, *dManager,
		                                              globalDSetComponent->fxaaDSets);
	           });

	// Present barrier
	rg.addPass("Present", {}, {{swapChainHandle, RGResourceUsage::Present}}, {}, nullptr);

	rg.compile();

	frame.commandBuffer.begin({});
	rg.execute(frame.commandBuffer);
	frame.commandBuffer.end();
}