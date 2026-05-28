#include "MainPass.hpp"

#include <Orhescyon/GeneralManager.hpp>

#include "../GraphicsContexts.hpp"
#include "../SwapChain.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../Components/BufferManagerComponent.hpp"
#include "../Components/ModelManagerComponent.hpp"
#include "../Components/TextureManagerComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Components/PipelineManagerComponent.hpp"
#include "../Components/GraphicsSettingsComponent.hpp"
#include "../Components/SkyboxComponent.hpp"
#include "../Components/DrawInfoComponent.hpp"
#include "../Components/RenderGraphComponent.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"
#include "../Resources/Components/ModelDSetComponent.hpp"
#include "../Resources/Components/BindlessTextureDSetComponent.hpp"
#include "../Resources/Managers/BufferManager.hpp"
#include "../Resources/Managers/ModelManager.hpp"
#include "../Resources/Managers/TextureManager.hpp"
#include "../Resources/Managers/Bindings.hpp"
#include "../Resources/Managers/DescriptorManager.hpp"
#include "../Resources/Managers/Vertex.hpp"
#include "../Managers/PipelineManager.hpp"
#include "../Factories/PipelineFactory.hpp"
#include "../RenderGraph/RenderGraph.hpp"

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

	auto makeForward = [&](int alphaTest, int ibl, vk::CompareOp depthOp)
	{
		return PipelineDescription{
		    .shaderPath = "shaders/standard_forward.spv",
		    .specializationValues = {alphaTest, ibl, gtaoEnabled},
		    .vertexBindings = {bindingDesc},
		    .vertexAttributes = std::vector<vk::VertexInputAttributeDescription>(attrDescs.begin(), attrDescs.end()),
		    .cullMode = vk::CullModeFlagBits::eBack,
		    .depthTest = true,
		    .depthWrite = false,
		    .depthOp = depthOp,
		    .colorAttachments = {PipelineFactory::blendedAttachment()},
		    .colorFormats = {swapChain.hdrFormat},
		    .depthFormat = depthFormat,
		    .rasterizationSamples = samples,
		    .setLayoutNames = mainLayouts,
		};
	};

	PipelineDescription skyboxDesc{
	    .shaderPath = "shaders/skybox.spv",
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
		pManager.rebuild(makeForward(0, 1, vk::CompareOp::eEqual), "standard_forward_opaque");
		pManager.rebuild(makeForward(0, 0, vk::CompareOp::eEqual), "standard_forward_opaque_no_ibl");
		pManager.rebuild(makeForward(1, 1, vk::CompareOp::eGreaterOrEqual), "standard_forward_alpha");
		pManager.rebuild(makeForward(1, 0, vk::CompareOp::eGreaterOrEqual), "standard_forward_alpha_no_ibl");
		pManager.rebuild(skyboxDesc, "skybox");
	}
	else
	{
		pManager.build(makeForward(0, 1, vk::CompareOp::eEqual), "standard_forward_opaque");
		pManager.build(makeForward(0, 0, vk::CompareOp::eEqual), "standard_forward_opaque_no_ibl");
		pManager.build(makeForward(1, 1, vk::CompareOp::eGreaterOrEqual), "standard_forward_alpha");
		pManager.build(makeForward(1, 0, vk::CompareOp::eGreaterOrEqual), "standard_forward_alpha_no_ibl");
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
	const std::string alphaPipeline = hasSkybox ? "standard_forward_alpha" : "standard_forward_alpha_no_ibl";

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

	uint32_t opaqueSingleCount = drawInfo.opaqueSingleCount;
	uint32_t opaqueDoubleCount = drawInfo.opaqueDoubleCount;
	uint32_t alphaSingleCount = drawInfo.alphaSingleCount;
	uint32_t alphaDoubleCount = drawInfo.alphaDoubleCount;

	uint32_t currentCommandOffset = 0;
	uint32_t currentCountBufferOffset = 0;

	if (opaqueSingleCount > 0 || opaqueDoubleCount > 0)
	{
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines[opaquePipeline].pipeline);

		if (opaqueSingleCount > 0)
		{
			cmd.setCullMode(vk::CullModeFlagBits::eBack);
			cmd.drawIndexedIndirectCount(bManager.buffers[objectDSetComponent.compactedDrawBuffer.id].buffer[frame],
			                             currentCommandOffset,
			                             bManager.buffers[objectDSetComponent.drawCountBuffer.id].buffer[frame],
			                             currentCountBufferOffset, opaqueSingleCount, commandStride);
			currentCommandOffset += opaqueSingleCount * commandStride;
		}
		currentCountBufferOffset += sizeof(uint32_t);

		if (opaqueDoubleCount > 0)
		{
			cmd.setCullMode(vk::CullModeFlagBits::eNone);
			cmd.drawIndexedIndirectCount(bManager.buffers[objectDSetComponent.compactedDrawBuffer.id].buffer[frame],
			                             currentCommandOffset,
			                             bManager.buffers[objectDSetComponent.drawCountBuffer.id].buffer[frame],
			                             currentCountBufferOffset, opaqueDoubleCount, commandStride);
			currentCommandOffset += opaqueDoubleCount * commandStride;
		}
		currentCountBufferOffset += sizeof(uint32_t);
	}
	else
	{
		currentCommandOffset += (opaqueSingleCount + opaqueDoubleCount) * commandStride;
		currentCountBufferOffset += 2 * sizeof(uint32_t);
	}

	if (hasSkybox)
	{
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["skybox"].pipeline);
		cmd.setCullMode(vk::CullModeFlagBits::eNone);
		cmd.draw(3, 1, 0, 0);
	}

	if (alphaSingleCount > 0 || alphaDoubleCount > 0)
	{
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines[alphaPipeline].pipeline);

		if (alphaSingleCount > 0)
		{
			cmd.setCullMode(vk::CullModeFlagBits::eBack);
			cmd.drawIndexedIndirectCount(bManager.buffers[objectDSetComponent.compactedDrawBuffer.id].buffer[frame],
			                             currentCommandOffset,
			                             bManager.buffers[objectDSetComponent.drawCountBuffer.id].buffer[frame],
			                             currentCountBufferOffset, alphaSingleCount, commandStride);
			currentCommandOffset += alphaSingleCount * commandStride;
		}
		currentCountBufferOffset += sizeof(uint32_t);

		if (alphaDoubleCount > 0)
		{
			cmd.setCullMode(vk::CullModeFlagBits::eNone);
			cmd.drawIndexedIndirectCount(bManager.buffers[objectDSetComponent.compactedDrawBuffer.id].buffer[frame],
			                             currentCommandOffset,
			                             bManager.buffers[objectDSetComponent.drawCountBuffer.id].buffer[frame],
			                             currentCountBufferOffset, alphaDoubleCount, commandStride);
		}
	}
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
