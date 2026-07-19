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
	auto& textureManager = *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	auto depthFormat = textureManager.findBestFormat();

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
	auto& pipelineManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& textureManager = *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	auto bindingDesc = Vertex::getBindingDescription();
	auto attrDescs = Vertex::getAttributeDescriptions();
	auto depthFormat = textureManager.findBestFormat();
	std::vector<std::string> mainLayouts = {"globalSet", "modelSet", "textureSet"};

	const bool a2c = samples != vk::SampleCountFlagBits::e1;

	auto makeDesc = [&](int alphaTest, bool useA2C)
	{
		return PipelineDescription{
		    .shaderPath = "depth_prepass.spv",
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
		pipelineManager.rebuild(makeDesc(0, false), "depth_prepass");
		pipelineManager.rebuild(makeDesc(1, a2c), "depth_prepass_alpha");
	}
	else
	{
		pipelineManager.build(makeDesc(0, false));
		pipelineManager.build(makeDesc(1, a2c), "depth_prepass_alpha");
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
                        DescriptorManagerComponent& descriptorManager, GlobalDSetComponent& globalDSetComponent,
                        BufferManager& bufferManager, ModelDSetComponent& objectDSetComponent,
                        BindlessTextureDSetComponent& bindlessTextureDSetComponent, ModelManager& modelManager,
                        const DrawInfoComponent& drawInfo, PipelineManager& pipelineManager)
{
	auto& opaquePipe = pipelineManager.pipelines["depth_prepass"];
	auto& alphaPipe = pipelineManager.pipelines["depth_prepass_alpha"];

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *opaquePipe.layout, 0,
	                       descriptorManager.descriptorManager->getSet(globalDSetComponent.globalDSets, frame), nullptr);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *opaquePipe.layout, 1,
	                       descriptorManager.descriptorManager->getSet(objectDSetComponent.modelBufferDSet, frame), nullptr);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *opaquePipe.layout, 2,
	                       descriptorManager.descriptorManager->getSet(bindlessTextureDSetComponent.bindlessTextureSet), nullptr);

	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
	                                static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

	const uint32_t commandStride = sizeof(VkDrawIndexedIndirectCommand);
	cmd.bindVertexBuffers(0, modelManager.getVertexIndexBuffer(0).vertexBuffer, {0});
	cmd.bindIndexBuffer(modelManager.getVertexIndexBuffer(0).indexBuffer, 0,
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
			cmd.drawIndexedIndirectCount(bufferManager.getBuffer(objectDSetComponent.compactedDrawBuffer, frame),
			                             currentCommandOffset,
			                             bufferManager.getBuffer(objectDSetComponent.drawCountBuffer, frame),
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
	auto& descriptorManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& globalDSetComponent = *gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	auto& bufferManager = *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	auto& objectDSetComponent = *gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();
	auto& modelManager = *gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;
	auto& drawInfo = *gm.getContextComponent<CurrentFrameContext, DrawInfoComponent>();
	auto& pipelineManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& graphicsSettings = *gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>();
	auto& bindlessTextureDSetComponent = *gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();

	vk::ClearValue clearDepth0 = vk::ClearDepthStencilValue(0.0f, 0);
	vk::ClearValue clearBlack = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);

	std::vector<RGResourceAccess> mainWrites;
	std::vector<RGAttachmentConfig> colorAttachments;
	std::optional<RGAttachmentConfig> depthAttachment;

	vk::ResolveModeFlagBits colorResolve = vk::ResolveModeFlagBits::eAverage;
	vk::ResolveModeFlagBits depthResolve = vk::ResolveModeFlagBits::eSampleZero;

	if (graphicsSettings.msaaSamples & vk::SampleCountFlagBits::e1)
	{
		colorAttachments = {{"ViewNormals", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearBlack}};
		depthAttachment =
		    RGAttachmentConfig{"Depth", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearDepth0};
		mainWrites = {{"ViewNormals", RGResourceUsage::ColorAttachmentWrite},
		              {"Depth", RGResourceUsage::DepthAttachmentWrite}};
	}
	else
	{
		colorAttachments = {{"ViewNormalsMSAA", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearBlack,
		                     "ViewNormals", colorResolve}};
		depthAttachment = RGAttachmentConfig{
		    "DepthMSAA", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearDepth0, "Depth", depthResolve};
		mainWrites = {{"ViewNormalsMSAA", RGResourceUsage::ColorAttachmentWrite},
		              {"DepthMSAA", RGResourceUsage::DepthAttachmentWrite},
		              {"ViewNormals", RGResourceUsage::ColorAttachmentWrite},
		              {"Depth", RGResourceUsage::DepthAttachmentWrite}};
	}

	rg.addPass("DepthPrepass", {.colorAttachments = colorAttachments, .depthAttachment = depthAttachment}, {},
	           mainWrites,
	           [&, frame](vk::raii::CommandBuffer& cmd)
	           {
		           draw(cmd, frame, swapChain, descriptorManager, globalDSetComponent, bufferManager, objectDSetComponent,
		                bindlessTextureDSetComponent, modelManager, drawInfo, pipelineManager);
	           });
}
