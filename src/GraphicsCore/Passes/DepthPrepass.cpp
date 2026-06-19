#include "DepthPrepass.hpp"

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
#include "GraphicsCore/Components/DrawInfoComponent.hpp"
#include "GraphicsCore/Components/RenderGraphComponent.hpp"
#include "GraphicsCore/Resources/Components/GlobalDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/ModelDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include "GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "GraphicsCore/Resources/Managers/ModelManager.hpp"
#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "GraphicsCore/Resources/Managers/Vertex.hpp"
#include "GraphicsCore/Managers/PipelineManager.hpp"
#include "GraphicsCore/Factories/PipelineFactory.hpp"
#include "GraphicsCore/RenderGraph/RenderGraph.hpp"

void DepthPrepass::declareStreams(Orhescyon::GeneralManager& gm, vk::SampleCountFlagBits samples)
{
	auto& rg = *gm.getContextComponent<RenderGraphContext, RenderGraphComponent>()->renderGraph;
	auto& tManager = *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	auto depthFormat = tManager.findBestFormat();

	rg.declareLogicalStream("Depth", {depthFormat, RGSizeMode::FullExtent, vk::ImageAspectFlagBits::eDepth});
	rg.declareLogicalStream("ViewNormals",
	                        {vk::Format::eR16G16B16A16Sfloat, RGSizeMode::FullExtent, vk::ImageAspectFlagBits::eColor});
	rg.declareLogicalStream("DepthMSAA",
	                        {depthFormat, RGSizeMode::FullExtent, vk::ImageAspectFlagBits::eDepth, samples});
	rg.declareLogicalStream("ViewNormalsMSAA", {vk::Format::eR16G16B16A16Sfloat, RGSizeMode::FullExtent,
	                                            vk::ImageAspectFlagBits::eColor, samples});
}

void DepthPrepass::buildPipelines(Orhescyon::GeneralManager& gm, vk::SampleCountFlagBits samples, bool rebuild)
{
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& tManager = *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	auto bindingDesc = Vertex::getBindingDescription();
	auto attrDescs = Vertex::getAttributeDescriptions();
	auto depthFormat = tManager.findBestFormat();
	std::vector<std::string> mainLayouts = {"globalSet", "modelSet", "textureSet"};

	const bool a2c = samples != vk::SampleCountFlagBits::e1;

	auto makeDesc = [&](int alphaTest, bool useA2C)
	{
		return PipelineDescription{
		    .shaderPath = HALCYON_SHADER_OUT_DIR "/depth_prepass.spv",
		    .specializationValues = {alphaTest, useA2C ? 1 : 0},
		    .vertexBindings = {bindingDesc},
		    .vertexAttributes = std::vector<vk::VertexInputAttributeDescription>(attrDescs.begin(), attrDescs.end()),
		    .cullMode = vk::CullModeFlagBits::eBack,
		    .depthTest = true,
		    .depthWrite = true,
		    .depthOp = vk::CompareOp::eGreater,
		    .colorAttachments = {PipelineFactory::opaqueAttachment()},
		    .colorFormats = {vk::Format::eR16G16B16A16Sfloat},
		    .depthFormat = depthFormat,
		    .rasterizationSamples = samples,
		    .alphaToCoverage = useA2C,
		    .setLayoutNames = mainLayouts,
		};
	};

	if (rebuild)
	{
		pManager.rebuild(makeDesc(0, false), "depth_prepass");
		pManager.rebuild(makeDesc(1, a2c), "depth_prepass_alpha");
	}
	else
	{
		pManager.build(makeDesc(0, false));
		pManager.build(makeDesc(1, a2c), "depth_prepass_alpha");
	}
}

void DepthPrepass::onInit(Orhescyon::GeneralManager& gm)
{
	auto& settings = *gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>();
	declareStreams(gm, settings.msaaSamples);
	buildPipelines(gm, settings.msaaSamples, false);
}

void DepthPrepass::onSettingsChanged(Orhescyon::GeneralManager& gm)
{
	auto& settings = *gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>();
	if (settings.msaaSamples == settings.appliedMsaaSamples) return;
	declareStreams(gm, settings.msaaSamples);
	buildPipelines(gm, settings.msaaSamples, true);
}

void DepthPrepass::draw(vk::raii::CommandBuffer& cmd, uint32_t frame, SwapChain& swapChain,
                        DescriptorManagerComponent& dManager, GlobalDSetComponent& globalDSetComponent,
                        BufferManager& bManager, ModelDSetComponent& objectDSetComponent,
                        BindlessTextureDSetComponent& bindlessTextureDSetComponent, ModelManager& mManager,
                        const DrawInfoComponent& drawInfo, PipelineManager& pManager)
{
	auto& opaquePipe = pManager.pipelines["depth_prepass"];
	auto& alphaPipe = pManager.pipelines["depth_prepass_alpha"];

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *opaquePipe.layout, 0,
	                       dManager.descriptorManager->getSet(globalDSetComponent.globalDSets, frame), nullptr);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *opaquePipe.layout, 1,
	                       dManager.descriptorManager->getSet(objectDSetComponent.modelBufferDSet, frame), nullptr);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *opaquePipe.layout, 2,
	                       dManager.descriptorManager->getSet(bindlessTextureDSetComponent.bindlessTextureSet), nullptr);

	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
	                                static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

	const uint32_t commandStride = sizeof(VkDrawIndexedIndirectCommand);
	cmd.bindVertexBuffers(0, mManager.vertexIndexBuffers[mManager.meshes[0].vertexIndexBufferID].vertexBuffer, {0});
	cmd.bindIndexBuffer(mManager.vertexIndexBuffers[mManager.meshes[0].vertexIndexBufferID].indexBuffer, 0,
	                    vk::IndexType::eUint32);
	// Segments 0,1 = opaque, 2,3 = mask
	const uint32_t segmentCounts[4] = {drawInfo.opaqueSingleCount, drawInfo.opaqueDoubleCount, drawInfo.maskSingleCount,
	                                   drawInfo.maskDoubleCount};

	uint32_t currentCommandOffset = 0;
	uint32_t currentCountBufferOffset = 0;

	auto drawSegment = [&](uint32_t seg)
	{
		uint32_t count = segmentCounts[seg];
		if (count > 0)
		{
			cmd.setCullMode((seg & 1) ? vk::CullModeFlagBits::eNone : vk::CullModeFlagBits::eBack);
			cmd.drawIndexedIndirectCount(bManager.buffers[objectDSetComponent.compactedDrawBuffer.id].buffer[frame],
			                             currentCommandOffset,
			                             bManager.buffers[objectDSetComponent.drawCountBuffer.id].buffer[frame],
			                             currentCountBufferOffset, count, commandStride);
			currentCommandOffset += count * commandStride;
		}
		currentCountBufferOffset += sizeof(uint32_t);
	};

	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *opaquePipe.pipeline);
	drawSegment(0);
	drawSegment(1);

	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *alphaPipe.pipeline);
	drawSegment(2);
	drawSegment(3);
}

void DepthPrepass::addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame)
{
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& globalDSetComponent = *gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	auto& bManager = *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	auto& objectDSetComponent = *gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();
	auto& mManager = *gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;
	auto& drawInfo = *gm.getContextComponent<CurrentFrameContext, DrawInfoComponent>();
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& graphicsSettings = *gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>();
	auto& bindlessTextureDSetComponent = *gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();

	vk::ClearValue clearDepth0 = vk::ClearDepthStencilValue(0.0f, 0);
	vk::ClearValue clearBlack = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);

	if (graphicsSettings.msaaSamples & vk::SampleCountFlagBits::e1)
	{
		rg.addPass(
		    "DepthPrepass",
		    {.colorAttachments = {{"ViewNormals", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
		                           clearBlack}},
		     .depthAttachment =
		         RGAttachmentConfig{"Depth", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearDepth0}},
		    {},
		    {{"ViewNormals", RGResourceUsage::ColorAttachmentWrite}, {"Depth", RGResourceUsage::DepthAttachmentWrite}},
		    [&, frame](vk::raii::CommandBuffer& cmd)
		    {
			    draw(cmd, frame, swapChain, dManager, globalDSetComponent, bManager, objectDSetComponent,
			         bindlessTextureDSetComponent, mManager, drawInfo, pManager);
		    });
	}
	else
	{
		vk::ResolveModeFlagBits colorResolve = vk::ResolveModeFlagBits::eAverage;
		vk::ResolveModeFlagBits depthResolve = vk::ResolveModeFlagBits::eSampleZero;
		rg.addPass(
		    "DepthPrepass",
		    {.colorAttachments = {{"ViewNormalsMSAA", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
		                           clearBlack, "ViewNormals", colorResolve}},
		     .depthAttachment = RGAttachmentConfig{"DepthMSAA", vk::AttachmentLoadOp::eClear,
		                                           vk::AttachmentStoreOp::eStore, clearDepth0, "Depth", depthResolve}},
		    {},
		    {{"ViewNormalsMSAA", RGResourceUsage::ColorAttachmentWrite},
		     {"DepthMSAA", RGResourceUsage::DepthAttachmentWrite},
		     {"ViewNormals", RGResourceUsage::ColorAttachmentWrite},
		     {"Depth", RGResourceUsage::DepthAttachmentWrite}},
		    [&, frame](vk::raii::CommandBuffer& cmd)
		    {
			    draw(cmd, frame, swapChain, dManager, globalDSetComponent, bManager, objectDSetComponent,
			         bindlessTextureDSetComponent, mManager, drawInfo, pManager);
		    });
	}
}
