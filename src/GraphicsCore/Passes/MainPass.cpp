#include "MainPass.hpp"
#include "GraphicsCore/Passes/PassCommands.hpp"
#include "GraphicsCore/Passes/DrawVariant.hpp"

#include <Orhescyon/GeneralManager.hpp>

#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/SwapChain.hpp"
#include "GraphicsCore/Components/SwapChainComponent.hpp"
#include "GraphicsCore/Components/BufferManagerComponent.hpp"
#include "GraphicsCore/Components/ModelManagerComponent.hpp"
#include "GraphicsCore/Components/TextureManagerComponent.hpp"
#include "GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include "GraphicsCore/Components/PipelineManagerComponent.hpp"
#include "GraphicsCore/Components/GraphicsSettingsComponent.hpp"
#include "GraphicsCore/Components/SkyboxComponent.hpp"
#include "GraphicsCore/Components/DrawInfoComponent.hpp"
#include "GraphicsCore/Components/RenderGraphComponent.hpp"
#include "GraphicsCore/Resources/Components/GlobalDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/ModelDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include "GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "GraphicsCore/Resources/Managers/ModelManager.hpp"
#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "Shared/Bindings.h"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "GraphicsCore/Resources/Managers/Vertex.hpp"
#include "GraphicsCore/Managers/PipelineManager.hpp"
#include "GraphicsCore/Factories/PipelineFactory.hpp"
#include "GraphicsCore/RenderGraph/RenderGraph.hpp"

void MainPass::declareStreams(Orhescyon::GeneralManager& gm, vk::SampleCountFlagBits samples)
{
	auto& rg = *gm.getContextComponent<RenderGraphContext, RenderGraphComponent>()->renderGraph;
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;

	rg.declareLogicalStream("MainColor", {swapChain.hdrFormat, RGSizeMode::FullExtent, vk::ImageAspectFlagBits::eColor});
	rg.declareLogicalStream("MainColorMSAA",
	                        {swapChain.hdrFormat, RGSizeMode::FullExtent, vk::ImageAspectFlagBits::eColor, samples});
}

void MainPass::buildPipelines(Orhescyon::GeneralManager& gm, vk::SampleCountFlagBits samples, int gtaoEnabled,
                              bool rebuild)
{
	auto& pipelineManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& textureManager = *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto bindingDesc = Vertex::getBindingDescription();
	auto attrDescs = Vertex::getAttributeDescriptions();
	auto depthFormat = textureManager.findBestFormat();
	std::vector<std::string> mainLayouts = {"globalSet", "modelSet", "textureSet"};

	const bool a2c = samples != vk::SampleCountFlagBits::e1;

	auto makeForward = [&](int alphaTest, int ibl, bool useA2C, vk::CompareOp depthOp)
	{
		return PipelineDescription{
		    .shaderPath = "standard_forward.spv",
		    .specializationValues = {alphaTest, ibl, gtaoEnabled, useA2C ? 1 : 0},
		    .vertexBindings = {bindingDesc},
		    .vertexAttributes = std::vector<vk::VertexInputAttributeDescription>(attrDescs.begin(), attrDescs.end()),
		    .cullMode = vk::CullModeFlagBits::eBack,
		    .depthTest = true,
		    .depthWrite = false,
		    .depthOp = depthOp,
		    .colorAttachments = {useA2C ? PipelineFactory::opaqueAttachment() : PipelineFactory::blendedAttachment()},
		    .colorFormats = {swapChain.hdrFormat},
		    .depthFormat = depthFormat,
		    .rasterizationSamples = samples,
		    .alphaToCoverage = useA2C,
		    .setLayoutNames = mainLayouts,
		};
	};

	PipelineDescription skyboxDesc{
	    .shaderPath = "skybox.spv",
	    .cullMode = vk::CullModeFlagBits::eNone,
	    .depthTest = true,
	    .depthWrite = false,
	    .depthOp = vk::CompareOp::eEqual,
	    .colorAttachments = {PipelineFactory::blendedAttachment()},
	    .colorFormats = {swapChain.hdrFormat},
	    .depthFormat = depthFormat,
	    .rasterizationSamples = samples,
	    .setLayoutNames = mainLayouts,
	};

	if (rebuild)
	{
		pipelineManager.rebuild(makeForward(0, 1, false, vk::CompareOp::eEqual), "standard_opaque_forward");
		pipelineManager.rebuild(makeForward(0, 0, false, vk::CompareOp::eEqual), "standard_opaque_forward_no_ibl");
		pipelineManager.rebuild(makeForward(1, 1, a2c, vk::CompareOp::eGreaterOrEqual), "standard_mask_forward");
		pipelineManager.rebuild(makeForward(1, 0, a2c, vk::CompareOp::eGreaterOrEqual), "standard_mask_forward_no_ibl");
		pipelineManager.rebuild(makeForward(0, 1, false, vk::CompareOp::eGreaterOrEqual), "standard_blend_forward");
		pipelineManager.rebuild(makeForward(0, 0, false, vk::CompareOp::eGreaterOrEqual), "standard_blend_forward_no_ibl");
		pipelineManager.rebuild(skyboxDesc, "skybox");
	}
	else
	{
		pipelineManager.build(makeForward(0, 1, false, vk::CompareOp::eEqual), "standard_opaque_forward");
		pipelineManager.build(makeForward(0, 0, false, vk::CompareOp::eEqual), "standard_opaque_forward_no_ibl");
		pipelineManager.build(makeForward(1, 1, a2c, vk::CompareOp::eGreaterOrEqual), "standard_mask_forward");
		pipelineManager.build(makeForward(1, 0, a2c, vk::CompareOp::eGreaterOrEqual), "standard_mask_forward_no_ibl");
		pipelineManager.build(makeForward(0, 1, false, vk::CompareOp::eGreaterOrEqual), "standard_blend_forward");
		pipelineManager.build(makeForward(0, 0, false, vk::CompareOp::eGreaterOrEqual), "standard_blend_forward_no_ibl");
		pipelineManager.build(skyboxDesc);
	}
}

void MainPass::onInit(Orhescyon::GeneralManager& gm)
{
	auto& settings = *gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>();
	const int gtaoEnabled = settings.enableGtao ? 1 : 0;
	declareStreams(gm, settings.msaaSamples);
	buildPipelines(gm, settings.msaaSamples, gtaoEnabled, false);
}

void MainPass::onSettingsChanged(Orhescyon::GeneralManager& gm)
{
	auto& settings = *gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>();
	const bool msaaChanged = settings.msaaSamples != settings.appliedMsaaSamples;
	const bool gtaoChanged = settings.enableGtao != settings.appliedGtao;
	if (!msaaChanged && !gtaoChanged) return;

	const int gtaoEnabled = settings.enableGtao ? 1 : 0;
	if (msaaChanged) declareStreams(gm, settings.msaaSamples);
	buildPipelines(gm, settings.msaaSamples, gtaoEnabled, true);
}

void MainPass::draw(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, uint32_t frame,
                    BindlessTextureDSetComponent& bindlessTextureDSetComponent, DescriptorManagerComponent& descriptorManager,
                    GlobalDSetComponent& globalDSetComponent, BufferManager& bufferManager,
                    ModelDSetComponent& objectDSetComponent, ModelManager& modelManager, const DrawInfoComponent& drawInfo,
                    PipelineManager& pipelineManager, bool hasSkybox)
{
	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
	                                static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

	const std::string_view iblSuffix = hasSkybox ? "_forward" : "_forward_no_ibl";

	auto& firstLayout = pipelineManager.pipelines["standard_opaque_forward"].layout;
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *firstLayout, 0,
	                       descriptorManager.descriptorManager->getSet(globalDSetComponent.globalDSets, frame), nullptr);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *firstLayout, 1,
	                       descriptorManager.descriptorManager->getSet(objectDSetComponent.modelBufferDSet, frame), nullptr);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *firstLayout, 2,
	                       descriptorManager.descriptorManager->getSet(bindlessTextureDSetComponent.bindlessTextureSet), nullptr);

	cmd.bindVertexBuffers(0, modelManager.getVertexIndexBuffer(0).vertexBuffer, {0});
	cmd.bindIndexBuffer(modelManager.getVertexIndexBuffer(0).indexBuffer, 0,
	                    vk::IndexType::eUint32);

	if (hasSkybox)
	{
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineManager.pipelines["skybox"].pipeline);
		cmd.setCullMode(vk::CullModeFlagBits::eNone);
		cmd.draw(3, 1, 0, 0);
	}

	DrawCursor cursor{bufferManager.getBuffer(objectDSetComponent.compactedDrawBuffer, frame),
	                  bufferManager.getBuffer(objectDSetComponent.drawCountBuffer, frame)};

	std::string_view prevPipeline;
	for (auto& seg : drawInfo.segments)
	{
		auto& var = kDrawVariants[seg.variantIndex];
		std::string key = std::string(var.pipeline) + std::string(iblSuffix);
		if (var.pipeline != prevPipeline)
		{
			cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineManager.pipelines[key].pipeline);
			prevPipeline = var.pipeline;
		}
		cursor.draw(cmd, seg.maxCount, var.cullMode);
	}
}

void MainPass::addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame)
{
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto& descriptorManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& globalDSetComponent = *gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	auto& bufferManager = *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	auto& objectDSetComponent = *gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();
	auto& modelManager = *gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;
	auto& drawInfo = *gm.getContextComponent<CurrentFrameContext, DrawInfoComponent>();
	auto& pipelineManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& bindlessTextureDSetComponent = *gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
	auto& graphicsSettings = *gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>();
	bool hasSkybox = gm.getContextComponent<SkyBoxContext, SkyboxComponent>()->hasSkybox;

	vk::ClearValue clearSky = vk::ClearColorValue(0.0f, 0.637f, 1.0f, 1.0f);
	vk::ClearValue clearDepth0 = vk::ClearDepthStencilValue(0.0f, 0);

	std::vector<RGResourceAccess> reads = {{"shadowMap", RGResourceUsage::ShaderRead}};
	if (graphicsSettings.enableGtao) reads.push_back({"GTAOTexture", RGResourceUsage::ShaderRead});

	std::vector<RGResourceAccess> mainWrites;
	std::vector<RGAttachmentConfig> colorAttachments;
	std::optional<RGAttachmentConfig> depthAttachment;
	vk::ResolveModeFlagBits colorResolve = vk::ResolveModeFlagBits::eAverage;

	if (graphicsSettings.msaaSamples & vk::SampleCountFlagBits::e1)
	{
		mainWrites = {{"MainColor", RGResourceUsage::ColorAttachmentWrite},
		              {"Depth", RGResourceUsage::DepthAttachmentWrite}};
		colorAttachments = {{"MainColor", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearSky}};
		depthAttachment =
		    RGAttachmentConfig{"Depth", vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, clearDepth0};
	}
	else
	{
		mainWrites = {{"MainColorMSAA", RGResourceUsage::ColorAttachmentWrite},
		              {"DepthMSAA", RGResourceUsage::DepthAttachmentWrite},
		              {"MainColor", RGResourceUsage::ColorAttachmentWrite}};
		colorAttachments = {{"MainColorMSAA", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearSky,
		                     "MainColor", colorResolve}};
		depthAttachment =
		    RGAttachmentConfig{"DepthMSAA", vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, clearDepth0};
	}

	rg.addPass(
	    "Main", {.colorAttachments = colorAttachments, .depthAttachment = depthAttachment}, reads, std::move(mainWrites),
	    [&, frame, hasSkybox](vk::raii::CommandBuffer& cmd)
	    {
		    draw(cmd, swapChain, frame, bindlessTextureDSetComponent, descriptorManager, globalDSetComponent, bufferManager,
		         objectDSetComponent, modelManager, drawInfo, pipelineManager, hasSkybox);
	    },
	    [&descriptorManager, &globalDSetComponent, &graphicsSettings](const RenderGraph& graph, const RGPass& pass)
	    {
		    if (!graphicsSettings.enableGtao) return;
		    auto h = pass.getPhysicalRead("GTAOTexture");
		    descriptorManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.globalDSets,
		                                                        BIND_GLOBAL_GTAO_TEXTURE, graph.getImageView(h),
		                                                        graph.getSampler(h));
	    });
}
