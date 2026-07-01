#include "ToneMappingPass.hpp"

#include <Orhescyon/GeneralManager.hpp>

#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/SwapChain.hpp"
#include "GraphicsCore/Components/SwapChainComponent.hpp"
#include "GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include "GraphicsCore/Components/PipelineManagerComponent.hpp"
#include "GraphicsCore/Components/RenderGraphComponent.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "GraphicsCore/Managers/PipelineManager.hpp"
#include "GraphicsCore/Factories/PipelineFactory.hpp"
#include "GraphicsCore/RenderGraph/RenderGraph.hpp"
#include "GraphicsCore/Components/ExposureBufferComponent.hpp"
#include "GraphicsCore/Components/BufferManagerComponent.hpp"
#include "GraphicsCore/Components/GraphicsSettingsComponent.hpp"

namespace
{
enum Binding : uint32_t
{
	OffscreenInput = 0,
};
}

void ToneMappingPass::onInit(Orhescyon::GeneralManager& gm)
{
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto& rg = *gm.getContextComponent<RenderGraphContext, RenderGraphComponent>()->renderGraph;

	// PostProcessColor is the shared post-process target
	rg.declareLogicalStream("PostProcessColor",
	                        {swapChain.swapChainImageFormat, RGSizeMode::FullExtent, vk::ImageAspectFlagBits::eColor});
	rg.setTerminalOutput("PostProcessColor", "swapChainImage");

	_dSetMainColor = dManager.allocate("screenSpaceSet");
	_dSetExposure = dManager.allocate("exposureSet", MAX_FRAMES_IN_FLIGHT);

	auto& exposureComp = *gm.getContextComponent<ExposureBufferContext, ExposureBufferComponent>();
	auto& bManager = *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		dManager.update(_dSetExposure, 1, i, vk::DescriptorType::eStorageBuffer,
		                bManager.buffers[exposureComp.exposureBuffer.id].buffer[0]);
	}

	pManager.build(PipelineDescription{
	    .shaderPath = "tone_mapping.spv",
	    .specializationValues = {1},
	    .cullMode = vk::CullModeFlagBits::eNone,
	    .colorAttachments = {PipelineFactory::opaqueAttachment()},
	    .colorFormats = {swapChain.swapChainImageFormat},
	    .setLayoutNames = {"screenSpaceSet", "exposureSet"},
	});
}

void ToneMappingPass::onSettingsChanged(Orhescyon::GeneralManager& gm)
{
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto& settings = *gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>();

	pManager.rebuild(PipelineDescription{
	    .shaderPath = "tone_mapping.spv",
	    .specializationValues = {settings.enableAutoExposure ? 1 : 0},
	    .cullMode = vk::CullModeFlagBits::eNone,
	    .colorAttachments = {PipelineFactory::opaqueAttachment()},
	    .colorFormats = {swapChain.swapChainImageFormat},
	    .setLayoutNames = {"screenSpaceSet", "exposureSet"},
	}, "tone_mapping");
}

void ToneMappingPass::addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame)
{
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;

	vk::ClearValue clearBlack = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);
	rg.addPass(
	    "ToneMapping",
	    {.colorAttachments = {{"PostProcessColor", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
	                           clearBlack}}},
	    {{"MainColor", RGResourceUsage::ShaderRead}}, {{"PostProcessColor", RGResourceUsage::ColorAttachmentWrite}},
	    [&, frame,dset = _dSetMainColor](vk::raii::CommandBuffer& cmd)
	    {
		    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["tone_mapping"].pipeline);

		    cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
		                                    static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
		    cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

		    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["tone_mapping"].layout, 0,
		                           dManager.descriptorManager->getSet(dset), nullptr);
		    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["tone_mapping"].layout, 1,
		                           dManager.descriptorManager->getSet(_dSetExposure, frame), nullptr);

		    cmd.setCullMode(vk::CullModeFlagBits::eNone);
		    cmd.draw(3, 1, 0, 0);
	    },
	    [&dManager, dset = _dSetMainColor](const RenderGraph& graph, const RGPass& pass)
	    {
		    auto colorHnd = pass.getPhysicalRead("MainColor");
		    dManager.descriptorManager->updateSingleTextureDSet(dset, Binding::OffscreenInput,
		                                                        graph.getImageView(colorHnd), graph.getSampler(colorHnd));
	    });
}
