#include "VignettePass.hpp"

#include <Orhescyon/GeneralManager.hpp>

#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/SwapChain.hpp"
#include "GraphicsCore/Components/SwapChainComponent.hpp"
#include "GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include "GraphicsCore/Components/PipelineManagerComponent.hpp"
#include "GraphicsCore/Components/GraphicsSettingsComponent.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "GraphicsCore/Managers/PipelineManager.hpp"
#include "GraphicsCore/Factories/PipelineFactory.hpp"
#include "GraphicsCore/RenderGraph/RenderGraph.hpp"

namespace
{
enum Binding : uint32_t
{
	OffscreenInput = 0,
};
}

bool VignettePass::isEnabled(Orhescyon::GeneralManager& gm) const
{
	return gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>()->enableVignette;
}

void VignettePass::onInit(Orhescyon::GeneralManager& gm)
{
	auto& descriptorManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	auto& pipelineManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;

	_dset = descriptorManager.allocate("screenSpaceSet");

	pipelineManager.build(PipelineDescription{
	    .shaderPath = "vignette.spv",
	    .cullMode = vk::CullModeFlagBits::eNone,
	    .colorAttachments = {PipelineFactory::opaqueAttachment()},
	    .colorFormats = {swapChain.swapChainImageFormat},
	    .setLayoutNames = {"screenSpaceSet"},
	});
}

void VignettePass::addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t /*frame*/)
{
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto& descriptorManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& pipelineManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;

	vk::ClearValue clearBlack = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);
	rg.addPass(
	    "Vignette",
	    {.colorAttachments = {{"PostProcessColor", vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
	                           clearBlack}}},
	    {{"PostProcessColor", RGResourceUsage::ShaderRead}},
	    {{"PostProcessColor", RGResourceUsage::ColorAttachmentWrite}},
	    [&, dset = _dset](vk::raii::CommandBuffer& cmd)
	    {
		    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineManager.pipelines["vignette"].pipeline);

		    cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
		                                    static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
		    cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

		    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineManager.pipelines["vignette"].layout, 0,
		                           descriptorManager.descriptorManager->getSet(dset), nullptr);

		    cmd.setCullMode(vk::CullModeFlagBits::eNone);
		    cmd.draw(3, 1, 0, 0);
	    },
	    [&descriptorManager, dset = _dset](const RenderGraph& graph, const RGPass& pass)
	    {
		    auto colorHnd = pass.getPhysicalRead("PostProcessColor");
		    descriptorManager.descriptorManager->updateSingleTextureDSet(dset, Binding::OffscreenInput,
		                                                        graph.getImageView(colorHnd), graph.getSampler(colorHnd));
	    });
}
