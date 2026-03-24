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
#include "../Components/GraphicsSettingsComponent.hpp"
#include "../Resources/Managers/Bindings.hpp"
#include "../GraphicsInit.hpp"

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
	GraphicsSettingsComponent* graphicsSettings =
	    gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>();
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

	if (graphicsSettings->msaaSamples != graphicsSettings->appliedMsaaSamples)
	{
		GraphicsInit::recreateMsaaPipelines(gm, graphicsSettings->msaaSamples);
		graphicsSettings->appliedMsaaSamples = graphicsSettings->msaaSamples;
	}

	rg.setTerminalOutput("PostProcessColor", "swapChainImage"); // The final target for post-process chains
	rg.setTerminalOutput("shadowMap",
	                     "shadowMap"); // The shadow pass writes directly to the imported physical shadow map

	vk::ClearValue clearSky = vk::ClearColorValue(0.0f, 0.637f, 1.0f, 1.0f);
	vk::ClearValue clearBlack = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);
	vk::ClearValue clearWhite = vk::ClearColorValue(1.0f, 1.0f, 1.0f, 1.0f);
	vk::ClearValue clearDepth0 = vk::ClearDepthStencilValue(0.0f, 0);

	rg.addPass("ShadowCull", {.isCompute = true}, {}, {},
	           [&](vk::raii::CommandBuffer& cmd)
	           {
		           CommandBufferFactory::drawShadowCullPass(cmd, pipelineHandler, currentFrame, *dManager,
		                                                    globalDSetComponent, objectDSetComponent, modelManager,
		                                                    bufferManager, *drawInfo);
	           });

	rg.addPass("Shadow",
	           {.depthAttachment = RGAttachmentConfig{"shadowMap", vk::AttachmentLoadOp::eClear,
	                                                  vk::AttachmentStoreOp::eStore, clearDepth0},
	            .customExtent = vk::Extent2D{static_cast<uint32_t>(lightTexture->sizeX),
	                                         static_cast<uint32_t>(lightTexture->sizeY)}},
	           {}, {{"shadowMap", RGResourceUsage::DepthAttachmentWrite}},
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

	if (graphicsSettings->msaaSamples & vk::SampleCountFlagBits::e1) // What a mess
	{
		rg.addPass("DepthPrepass",
		           {.depthAttachment = RGAttachmentConfig{"Depth", vk::AttachmentLoadOp::eClear,
		                                                  vk::AttachmentStoreOp::eStore, clearDepth0}},
		           {}, {{"Depth", RGResourceUsage::DepthAttachmentWrite}},
		           [&](vk::raii::CommandBuffer& cmd)
		           {
			           CommandBufferFactory::drawDepthPrepass(cmd, swapChain, pipelineHandler, currentFrame,
			                                                  *materialDSetComponent, *dManager, globalDSetComponent,
			                                                  bufferManager, objectDSetComponent, modelManager, *drawInfo);
		           });

		std::vector<RGResourceAccess> mainWrites = {{"MainColor", RGResourceUsage::ColorAttachmentWrite},
		                                            {"ViewNormals", RGResourceUsage::ColorAttachmentWrite},
		                                            {"Depth", RGResourceUsage::DepthAttachmentWrite}};
		rg.addPass(
		    "Main",
		    {.colorAttachments = {{"MainColor", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearSky},
		                          {"ViewNormals", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
		                           clearBlack}},
		     .depthAttachment =
		         RGAttachmentConfig{"Depth", vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, clearDepth0}},
		    {{"shadowMap", RGResourceUsage::ShaderRead}}, std::move(mainWrites),
		    [&](vk::raii::CommandBuffer& cmd)
		    {
			    CommandBufferFactory::drawMainPass(cmd, swapChain, pipelineHandler, currentFrame, *materialDSetComponent,
			                                       *dManager, globalDSetComponent, bufferManager, objectDSetComponent,
			                                       modelManager, *drawInfo);
		    });
	}
	else
	{
		rg.addPass("DepthPrepass",
		           {.depthAttachment = RGAttachmentConfig{"DepthMSAA", vk::AttachmentLoadOp::eClear,
		                                                  vk::AttachmentStoreOp::eStore, clearDepth0}},
		           {}, {{"DepthMSAA", RGResourceUsage::DepthAttachmentWrite}},
		           [&](vk::raii::CommandBuffer& cmd)
		           {
			           CommandBufferFactory::drawDepthPrepass(cmd, swapChain, pipelineHandler, currentFrame,
			                                                  *materialDSetComponent, *dManager, globalDSetComponent,
			                                                  bufferManager, objectDSetComponent, modelManager, *drawInfo);
		           });

		std::vector<RGResourceAccess> mainWrites = {{"MainColorMSAA", RGResourceUsage::ColorAttachmentWrite},
		                                            {"ViewNormalsMSAA", RGResourceUsage::ColorAttachmentWrite},
		                                            {"DepthMSAA", RGResourceUsage::DepthAttachmentWrite},
		                                            {"MainColor", RGResourceUsage::ColorAttachmentWrite},
		                                            {"ViewNormals", RGResourceUsage::ColorAttachmentWrite},
		                                            {"Depth", RGResourceUsage::DepthAttachmentWrite}};

		vk::ResolveModeFlagBits colorResolve = vk::ResolveModeFlagBits::eAverage;
		vk::ResolveModeFlagBits depthResolve = vk::ResolveModeFlagBits::eSampleZero;
		rg.addPass(
		    "Main",
		    {.colorAttachments = {{"MainColorMSAA", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearSky,
		                           "MainColor", colorResolve},
		                          {"ViewNormalsMSAA", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
		                           clearBlack, "ViewNormals", colorResolve}},
		     .depthAttachment = RGAttachmentConfig{"DepthMSAA", vk::AttachmentLoadOp::eLoad,
		                                           vk::AttachmentStoreOp::eStore, clearDepth0, "Depth", depthResolve}},
		    {{"shadowMap", RGResourceUsage::ShaderRead}}, std::move(mainWrites),
		    [&](vk::raii::CommandBuffer& cmd)
		    {
			    CommandBufferFactory::drawMainPass(cmd, swapChain, pipelineHandler, currentFrame, *materialDSetComponent,
			                                       *dManager, globalDSetComponent, bufferManager, objectDSetComponent,
			                                       modelManager, *drawInfo);
		    });
	}
	

	if (graphicsSettings->enableSsao)
	{
		rg.addPass(
		    "SSAO",
		    {.colorAttachments = {{"SSAOTexture", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
		                           clearWhite}}},
		    {{"Depth", RGResourceUsage::ShaderRead}, {"ViewNormals", RGResourceUsage::ShaderRead}},
		    {{"SSAOTexture", RGResourceUsage::ColorAttachmentWrite}},
		    [&](vk::raii::CommandBuffer& cmd)
		    {
			    CommandBufferFactory::drawSsaoPass(cmd, swapChain, pipelineHandler, *dManager,
			                                       globalDSetComponent->ssaoDSets, globalDSetComponent->globalDSets,
			                                       *ssaoSettings);
		    },
		    [dManager, globalDSetComponent](const RenderGraph& graph, const RGPass& pass)
		    {
			    auto depthHnd = pass.getPhysicalRead("Depth");
			    auto normHnd = pass.getPhysicalRead("ViewNormals");
			    dManager->descriptorManager->updateSingleTextureDSet(
			        globalDSetComponent->ssaoDSets, Bindings::SSAO::DepthInput, graph.getImageView(depthHnd),
			        graph.getSampler(depthHnd));
			    dManager->descriptorManager->updateSingleTextureDSet(
			        globalDSetComponent->ssaoDSets, Bindings::SSAO::NormalsInput, graph.getImageView(normHnd),
			        graph.getSampler(normHnd));
		    });

		rg.addPass(
		    "SSAOBlur",
		    {.colorAttachments = {{"SSAOBlurTexture", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
		                           clearWhite}}},
		    {{"SSAOTexture", RGResourceUsage::ShaderRead},
		     {"Depth", RGResourceUsage::ShaderRead},
		     {"ViewNormals", RGResourceUsage::ShaderRead}},
		    {{"SSAOBlurTexture", RGResourceUsage::ColorAttachmentWrite}},
		    [&](vk::raii::CommandBuffer& cmd)
		    {
			    CommandBufferFactory::drawSsaoBlurPass(cmd, swapChain, pipelineHandler, *dManager,
			                                           globalDSetComponent->ssaoBlurDSets);
		    },
		    [dManager, globalDSetComponent](const RenderGraph& graph, const RGPass& pass)
		    {
			    auto ssaoHnd = pass.getPhysicalRead("SSAOTexture");
			    auto depthHnd = pass.getPhysicalRead("Depth");
			    auto normHnd = pass.getPhysicalRead("ViewNormals");
			    dManager->descriptorManager->updateSingleTextureDSet(
			        globalDSetComponent->ssaoBlurDSets, Bindings::SSAOBlur::SsaoInput, graph.getImageView(ssaoHnd),
			        graph.getSampler(ssaoHnd));
			    dManager->descriptorManager->updateSingleTextureDSet(
			        globalDSetComponent->ssaoBlurDSets, Bindings::SSAOBlur::DepthInput, graph.getImageView(depthHnd),
			        graph.getSampler(depthHnd));
			    dManager->descriptorManager->updateSingleTextureDSet(
			        globalDSetComponent->ssaoBlurDSets, Bindings::SSAOBlur::NormalsInput, graph.getImageView(normHnd),
			        graph.getSampler(normHnd));
		    });

		rg.addPass(
		    "SSAOApply",
		    {.colorAttachments = {{"MainColor", vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, clearBlack}}},
		    {{"MainColor", RGResourceUsage::ShaderRead}, {"SSAOBlurTexture", RGResourceUsage::ShaderRead}},
		    {{"MainColor", RGResourceUsage::ColorAttachmentWrite}}, // Overwrites MainColor!
		    [&](vk::raii::CommandBuffer& cmd)
		    {
			    CommandBufferFactory::drawSSAOApplyPass(cmd, swapChain, pipelineHandler, *dManager,
			                                            globalDSetComponent->ssaoApplyDSets);
		    },
		    [dManager, globalDSetComponent](const RenderGraph& graph, const RGPass& pass)
		    {
			    auto colorHnd = pass.getPhysicalRead("MainColor");
			    auto blurHnd = pass.getPhysicalRead("SSAOBlurTexture");
			    dManager->descriptorManager->updateSingleTextureDSet(
			        globalDSetComponent->ssaoApplyDSets, Bindings::SSAOApply::ColorInput, graph.getImageView(colorHnd),
			        graph.getSampler(colorHnd));
			    dManager->descriptorManager->updateSingleTextureDSet(
			        globalDSetComponent->ssaoApplyDSets, Bindings::SSAOApply::SsaoInput, graph.getImageView(blurHnd),
			        graph.getSampler(blurHnd));
		    });
	}

	rg.addPass(
	    "ToneMapping",
	    {.colorAttachments = {{"PostProcessColor", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
	                           clearBlack}}},
	    {{"MainColor", RGResourceUsage::ShaderRead}}, {{"PostProcessColor", RGResourceUsage::ColorAttachmentWrite}},
	    [&](vk::raii::CommandBuffer& cmd)
	    {
		    CommandBufferFactory::drawToneMappingPass(cmd, swapChain, pipelineHandler, *dManager,
		                                              globalDSetComponent->toneMappingDSets);
	    },
	    [dManager, globalDSetComponent](const RenderGraph& graph, const RGPass& pass)
	    {
		    auto colorHnd = pass.getPhysicalRead("MainColor");
		    dManager->descriptorManager->updateSingleTextureDSet(
		        globalDSetComponent->toneMappingDSets, Bindings::ToneMapping::OffscreenInput,
		        graph.getImageView(colorHnd), graph.getSampler(colorHnd));
	    });

	if (graphicsSettings->enableFxaa)
	{
		rg.addPass(
		    "FXAA",
		    {.colorAttachments = {{"PostProcessColor", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
		                           clearBlack}}},
		    {{"PostProcessColor", RGResourceUsage::ShaderRead}},
		    {{"PostProcessColor", RGResourceUsage::ColorAttachmentWrite}},
		    [&](vk::raii::CommandBuffer& cmd)
		    {
			    CommandBufferFactory::drawFxaaPass(cmd, swapChain, pipelineHandler, *dManager,
			                                       globalDSetComponent->fxaaDSets);
		    },
		    [dManager, globalDSetComponent](const RenderGraph& graph, const RGPass& pass)
		    {
			    auto colorHnd = pass.getPhysicalRead("PostProcessColor");
			    dManager->descriptorManager->updateSingleTextureDSet(
			        globalDSetComponent->fxaaDSets, Bindings::FXAA::ColorInput, graph.getImageView(colorHnd),
			        graph.getSampler(colorHnd));
		    });
	}

	rg.addPass("ImGui",
	           {.colorAttachments = {{"PostProcessColor", vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore}}},
	           {}, // Keep it empty, we dont read PostProcessColor in the shader, we just bind it as a render target. The
	               // ImGui writes to it directly.
	           {{"PostProcessColor", RGResourceUsage::ColorAttachmentWrite}},
	           [&](vk::raii::CommandBuffer& cmd) { CommandBufferFactory::drawImGui(cmd); });

	// Present barrier
	rg.addPass("Present", {}, {{"swapChainImage", RGResourceUsage::Present}}, {}, nullptr);

	rg.compile();

	frame.commandBuffer.begin({});
	rg.execute(frame.commandBuffer);
	frame.commandBuffer.end();
}