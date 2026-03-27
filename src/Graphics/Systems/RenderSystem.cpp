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
#include "../GraphicsInit/GraphicsInit.hpp"

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
	DirectLightComponent* lightTexture = gm.getContextComponent<SunContext, DirectLightComponent>();
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
	auto noiseHandle = rg.importImage(
	    "NoiseImage", textureManager.textures[swapChain.ssaoNoiseTextureHandle.id].textureImage,
	    textureManager.textures[swapChain.ssaoNoiseTextureHandle.id].textureImageView, vk::ImageAspectFlagBits::eColor);

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
		    {{"Depth", RGResourceUsage::ShaderRead}, 
		     {"ViewNormals", RGResourceUsage::ShaderRead},
		     {"NoiseImage", RGResourceUsage::ShaderRead}},
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

	// === Bloom ===
	if (graphicsSettings->enableBloom)
	{
		const std::string mipNames[5] = {"BloomMip0", "BloomMip1", "BloomMip2", "BloomMip3", "BloomMip4"};
		const std::string downSource[5] = {"MainColor", "BloomMip0", "BloomMip1", "BloomMip2", "BloomMip3"};

		// Divisors matching the RGSizeMode of each mip level
		const uint32_t divisors[5] = {2, 4, 8, 16, 32};

		float threshold = graphicsSettings->bloomThreshold;
		float knee = graphicsSettings->bloomKnee;
		float intensity = graphicsSettings->bloomIntensity;

		// Downsample
		for (int i = 0; i < 5; i++)
		{
			uint32_t sourceDivision = (i == 0) ? 1 : divisors[i - 1];
			uint32_t dstDivision = divisors[i];
			float srcW = static_cast<float>(swapChain.swapChainExtent.width / sourceDivision);
			float srcH = static_cast<float>(swapChain.swapChainExtent.height / sourceDivision);
			uint32_t dstW = swapChain.swapChainExtent.width / dstDivision;
			uint32_t dstH = swapChain.swapChainExtent.height / dstDivision;
			float texelX = 1.0f / srcW;
			float texelY = 1.0f / srcH;
			int isFirst = (i == 0) ? 1 : 0;
			int passIdx = i;

			rg.addPass(
			    "BloomDown" + std::to_string(i),
			    {.colorAttachments = {{mipNames[i], vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
			                           clearBlack}}},
			    {{downSource[i], RGResourceUsage::ShaderRead}}, {{mipNames[i], RGResourceUsage::ColorAttachmentWrite}},
			    [&, texelX, texelY, threshold, knee, isFirst, dstW, dstH, passIdx](vk::raii::CommandBuffer& cmd)
			    {
				    CommandBufferFactory::drawBloomDownsamplePass(
				        cmd, pipelineHandler, *dManager, globalDSetComponent->bloomDownsampleDSets[passIdx], texelX, texelY,
				        threshold, knee, isFirst, vk::Extent2D{dstW, dstH});
			    },
			    [dManager, globalDSetComponent, passIdx, srcName = downSource[i]](const RenderGraph& graph,
			                                                                      const RGPass& pass)
			    {
				    auto srcHnd = pass.getPhysicalRead(srcName);
				    dManager->descriptorManager->updateSingleTextureDSet(
				        globalDSetComponent->bloomDownsampleDSets[passIdx], Bindings::BloomDownsample::InputTexture,
				        graph.getImageView(srcHnd), graph.getSampler(srcHnd));
			    });
		}

		// Upsample
		const std::string upCurrentSrc[5] = {"BloomMip4", "BloomMip3", "BloomMip2", "BloomMip1", "BloomMip0"};
		const std::string upPrevDst[5] = {"BloomMip3", "BloomMip2", "BloomMip1", "BloomMip0", "MainColor"};

		// Divisors for the destination (previous / target)
		const uint32_t upDstDivisors[5] = {16, 8, 4, 2, 1};

		for (int i = 0; i < 5; i++)
		{
			uint32_t currentDivision = (i == 0) ? 32 : upDstDivisors[i - 1]; // current = source being upsampled
			uint32_t dstDivision = upDstDivisors[i];
			uint32_t dstW = swapChain.swapChainExtent.width / dstDivision;
			uint32_t dstH = swapChain.swapChainExtent.height / dstDivision;
			// Texel size of the CURRENT (lower-res source being upsampled)
			float curW = static_cast<float>(swapChain.swapChainExtent.width / currentDivision);
			float curH = static_cast<float>(swapChain.swapChainExtent.height / currentDivision);
			float texelX = 1.0f / curW;
			float texelY = 1.0f / curH;
			int isLast = (i == 4) ? 1 : 0;
			int passIdx = i;

			rg.addPass(
			    "BloomUp" + std::to_string(i),
			    {.colorAttachments = {{upPrevDst[i], vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore}}},
			    {{upCurrentSrc[i], RGResourceUsage::ShaderRead}, {upPrevDst[i], RGResourceUsage::ShaderRead}},
			    {{upPrevDst[i], RGResourceUsage::ColorAttachmentWrite}},
			    [&, texelX, texelY, intensity, isLast, dstW, dstH, passIdx](vk::raii::CommandBuffer& cmd)
			    {
				    CommandBufferFactory::drawBloomUpsamplePass(cmd, pipelineHandler, *dManager,
				                                                globalDSetComponent->bloomUpsampleDSets[passIdx], texelX,
				                                                texelY, intensity, isLast, vk::Extent2D{dstW, dstH});
			    },
			    [dManager, globalDSetComponent, passIdx, curName = upCurrentSrc[i],
			     prevName = upPrevDst[i]](const RenderGraph& graph, const RGPass& pass)
			    {
				    auto curHnd = pass.getPhysicalRead(curName);
				    auto prevHnd = pass.getPhysicalRead(prevName);
				    dManager->descriptorManager->updateSingleTextureDSet(
				        globalDSetComponent->bloomUpsampleDSets[passIdx], Bindings::BloomUpsample::CurrentTexture,
				        graph.getImageView(curHnd), graph.getSampler(curHnd));
				    dManager->descriptorManager->updateSingleTextureDSet(
				        globalDSetComponent->bloomUpsampleDSets[passIdx], Bindings::BloomUpsample::PreviousTexture,
				        graph.getImageView(prevHnd), graph.getSampler(prevHnd));
			    });
		}
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