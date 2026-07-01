#include "MainPass.hpp"

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
#include "GraphicsCore/Resources/Managers/Bindings.hpp"
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
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& tManager = *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto bindingDesc = Vertex::getBindingDescription();
	auto attrDescs = Vertex::getAttributeDescriptions();
	auto depthFormat = tManager.findBestFormat();
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
		pManager.rebuild(makeForward(0, 1, false, vk::CompareOp::eEqual), "standard_forward_opaque");
		pManager.rebuild(makeForward(0, 0, false, vk::CompareOp::eEqual), "standard_forward_opaque_no_ibl");
		pManager.rebuild(makeForward(1, 1, a2c, vk::CompareOp::eGreaterOrEqual), "standard_forward_mask");
		pManager.rebuild(makeForward(1, 0, a2c, vk::CompareOp::eGreaterOrEqual), "standard_forward_mask_no_ibl");
		pManager.rebuild(makeForward(0, 1, false, vk::CompareOp::eGreaterOrEqual), "standard_forward_blend");
		pManager.rebuild(makeForward(0, 0, false, vk::CompareOp::eGreaterOrEqual), "standard_forward_blend_no_ibl");
		pManager.rebuild(skyboxDesc, "skybox");
	}
	else
	{
		pManager.build(makeForward(0, 1, false, vk::CompareOp::eEqual), "standard_forward_opaque");
		pManager.build(makeForward(0, 0, false, vk::CompareOp::eEqual), "standard_forward_opaque_no_ibl");
		pManager.build(makeForward(1, 1, a2c, vk::CompareOp::eGreaterOrEqual), "standard_forward_mask");
		pManager.build(makeForward(1, 0, a2c, vk::CompareOp::eGreaterOrEqual), "standard_forward_mask_no_ibl");
		pManager.build(makeForward(0, 1, false, vk::CompareOp::eGreaterOrEqual), "standard_forward_blend");
		pManager.build(makeForward(0, 0, false, vk::CompareOp::eGreaterOrEqual), "standard_forward_blend_no_ibl");
		pManager.build(skyboxDesc);
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
                    BindlessTextureDSetComponent& bindlessTextureDSetComponent, DescriptorManagerComponent& dManager,
                    GlobalDSetComponent& globalDSetComponent, BufferManager& bManager,
                    ModelDSetComponent& objectDSetComponent, ModelManager& mManager, const DrawInfoComponent& drawInfo,
                    PipelineManager& pManager, bool hasSkybox)
{
	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
	                                static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

	const std::string opaquePipeline = hasSkybox ? "standard_forward_opaque" : "standard_forward_opaque_no_ibl";
	const std::string maskPipeline = hasSkybox ? "standard_forward_mask" : "standard_forward_mask_no_ibl";
	const std::string blendPipeline = hasSkybox ? "standard_forward_blend" : "standard_forward_blend_no_ibl";

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines[opaquePipeline].layout, 0,
	                       dManager.descriptorManager->getSet(globalDSetComponent.globalDSets, frame), nullptr);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines[opaquePipeline].layout, 1,
	                       dManager.descriptorManager->getSet(objectDSetComponent.modelBufferDSet, frame), nullptr);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines[opaquePipeline].layout, 2,
	                       dManager.descriptorManager->getSet(bindlessTextureDSetComponent.bindlessTextureSet), nullptr);

	const uint32_t commandStride = sizeof(VkDrawIndexedIndirectCommand);
	cmd.bindVertexBuffers(0, mManager.vertexIndexBuffers[mManager.meshes[0].vertexIndexBufferID].vertexBuffer, {0});
	cmd.bindIndexBuffer(mManager.vertexIndexBuffers[mManager.meshes[0].vertexIndexBufferID].indexBuffer, 0,
	                    vk::IndexType::eUint32);

	const uint32_t segmentCounts[6] = {drawInfo.opaqueSingleCount, drawInfo.opaqueDoubleCount,
	                                   drawInfo.maskSingleCount,   drawInfo.maskDoubleCount,
	                                   drawInfo.blendSingleCount,  drawInfo.blendDoubleCount};

	uint32_t currentCommandOffset = 0;
	uint32_t currentCountBufferOffset = 0;

	auto drawSegment = [&](uint32_t segment, bool backfaceCulling)
	{
		uint32_t count = segmentCounts[segment];
		if (count > 0)
		{
			cmd.setCullMode(backfaceCulling ? vk::CullModeFlagBits::eBack : vk::CullModeFlagBits::eNone);
			cmd.drawIndexedIndirectCount(bManager.buffers[objectDSetComponent.compactedDrawBuffer.id].buffer[frame],
			                             currentCommandOffset,
			                             bManager.buffers[objectDSetComponent.drawCountBuffer.id].buffer[frame],
			                             currentCountBufferOffset, count, commandStride);
			currentCommandOffset += count * commandStride;
		}
		currentCountBufferOffset += sizeof(uint32_t);
	};

	// OPAQUE
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines[opaquePipeline].pipeline);
	drawSegment(0, true);
	drawSegment(1, false);

	if (hasSkybox)
	{
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["skybox"].pipeline);
		cmd.setCullMode(vk::CullModeFlagBits::eNone);
		cmd.draw(3, 1, 0, 0);
	}

	// MASK
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines[maskPipeline].pipeline);
	drawSegment(2, true);
	drawSegment(3, false);

	// BLEND
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines[blendPipeline].pipeline);
	drawSegment(4, true);
	drawSegment(5, false);
}

void MainPass::addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame)
{
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& globalDSetComponent = *gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	auto& bManager = *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	auto& objectDSetComponent = *gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();
	auto& mManager = *gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;
	auto& drawInfo = *gm.getContextComponent<CurrentFrameContext, DrawInfoComponent>();
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& bindlessTextureDSetComponent = *gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
	auto& graphicsSettings = *gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>();
	bool hasSkybox = gm.getContextComponent<SkyBoxContext, SkyboxComponent>()->hasSkybox;

	vk::ClearValue clearSky = vk::ClearColorValue(0.0f, 0.637f, 1.0f, 1.0f);
	vk::ClearValue clearDepth0 = vk::ClearDepthStencilValue(0.0f, 0);

	std::vector<RGResourceAccess> reads = {{"shadowMap", RGResourceUsage::ShaderRead}};
	if (graphicsSettings.enableGtao) reads.push_back({"GTAOTexture", RGResourceUsage::ShaderRead});

	if (graphicsSettings.msaaSamples & vk::SampleCountFlagBits::e1) // What a mess
	{
		std::vector<RGResourceAccess> mainWrites = {{"MainColor", RGResourceUsage::ColorAttachmentWrite},
		                                            {"Depth", RGResourceUsage::DepthAttachmentWrite}};
		rg.addPass(
		    "Main",
		    {.colorAttachments = {{"MainColor", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearSky}},
		     .depthAttachment =
		         RGAttachmentConfig{"Depth", vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, clearDepth0}},
		    reads, std::move(mainWrites),
		    [&, frame, hasSkybox](vk::raii::CommandBuffer& cmd)
		    {
			    draw(cmd, swapChain, frame, bindlessTextureDSetComponent, dManager, globalDSetComponent, bManager,
			         objectDSetComponent, mManager, drawInfo, pManager, hasSkybox);
		    },
		    [&dManager, &globalDSetComponent, &graphicsSettings](const RenderGraph& graph, const RGPass& pass)
		    {
			    if (!graphicsSettings.enableGtao) return;
			    auto h = pass.getPhysicalRead("GTAOTexture");
			    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.globalDSets,
			                                                        Bindings::Global::GtaoTexture, graph.getImageView(h),
			                                                        graph.getSampler(h));
		    });
	}
	else
	{
		std::vector<RGResourceAccess> mainWrites = {{"MainColorMSAA", RGResourceUsage::ColorAttachmentWrite},
		                                            {"DepthMSAA", RGResourceUsage::DepthAttachmentWrite},
		                                            {"MainColor", RGResourceUsage::ColorAttachmentWrite}};

		vk::ResolveModeFlagBits colorResolve = vk::ResolveModeFlagBits::eAverage;
		rg.addPass(
		    "Main",
		    {.colorAttachments = {{"MainColorMSAA", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearSky,
		                           "MainColor", colorResolve}},
		     .depthAttachment = RGAttachmentConfig{"DepthMSAA", vk::AttachmentLoadOp::eLoad,
		                                           vk::AttachmentStoreOp::eStore, clearDepth0}},
		    reads, std::move(mainWrites),
		    [&, frame, hasSkybox](vk::raii::CommandBuffer& cmd)
		    {
			    draw(cmd, swapChain, frame, bindlessTextureDSetComponent, dManager, globalDSetComponent, bManager,
			         objectDSetComponent, mManager, drawInfo, pManager, hasSkybox);
		    },
		    [&dManager, &globalDSetComponent, &graphicsSettings](const RenderGraph& graph, const RGPass& pass)
		    {
			    if (!graphicsSettings.enableGtao) return;
			    auto h = pass.getPhysicalRead("GTAOTexture");
			    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.globalDSets,
			                                                        Bindings::Global::GtaoTexture, graph.getImageView(h),
			                                                        graph.getSampler(h));
		    });
	}
}
