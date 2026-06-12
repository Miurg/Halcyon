#include "FXAAPass.hpp"

#include <Orhescyon/GeneralManager.hpp>

#include "../GraphicsContexts.hpp"
#include "../SwapChain.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Components/PipelineManagerComponent.hpp"
#include "../Components/GraphicsSettingsComponent.hpp"
#include "../Resources/Managers/DescriptorManager.hpp"
#include "../Managers/PipelineManager.hpp"
#include "../Factories/PipelineFactory.hpp"
#include "../RenderGraph/RenderGraph.hpp"

namespace
{
enum Binding : uint32_t
{
	ColorInput = 0,
};
}

bool FXAAPass::isEnabled(Orhescyon::GeneralManager& gm) const
{
	return gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>()->enableFxaa;
}

void FXAAPass::onInit(Orhescyon::GeneralManager& gm)
{
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;

	_dset = dManager.allocate("screenSpaceSet");

	pManager.build(PipelineDescription{
	    .shaderPath = "shaders/fxaa.spv",
	    .cullMode = vk::CullModeFlagBits::eNone,
	    .colorAttachments = {PipelineFactory::opaqueAttachment()},
	    .colorFormats = {swapChain.swapChainImageFormat},
	    .setLayoutNames = {"screenSpaceSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eFragment, 0, sizeof(float) * 2}},
	});
}

void FXAAPass::addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t /*frame*/)
{
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;

	vk::ClearValue clearBlack = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);
	rg.addPass(
	    "FXAA",
	    {.colorAttachments = {{"PostProcessColor", vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
	                           clearBlack}}},
	    {{"PostProcessColor", RGResourceUsage::ShaderRead}},
	    {{"PostProcessColor", RGResourceUsage::ColorAttachmentWrite}},
	    [&, dset = _dset](vk::raii::CommandBuffer& cmd)
	    {
		    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["fxaa"].pipeline);

		    cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
		                                    static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
		    cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

		    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["fxaa"].layout, 0,
		                           dManager.descriptorManager->getSet(dset), nullptr);

		    struct PushConstants
		    {
			    float rcpFrame[2];
		    } push;
		    push.rcpFrame[0] = 1.0f / static_cast<float>(swapChain.swapChainExtent.width);
		    push.rcpFrame[1] = 1.0f / static_cast<float>(swapChain.swapChainExtent.height);

		    cmd.pushConstants<PushConstants>(*pManager.pipelines["fxaa"].layout, vk::ShaderStageFlagBits::eFragment, 0,
		                                     push);
		    cmd.setCullMode(vk::CullModeFlagBits::eNone);
		    cmd.draw(3, 1, 0, 0);
	    },
	    [&dManager, dset = _dset](const RenderGraph& graph, const RGPass& pass)
	    {
		    auto colorHnd = pass.getPhysicalRead("PostProcessColor");
		    dManager.descriptorManager->updateSingleTextureDSet(dset, Binding::ColorInput, graph.getImageView(colorHnd),
		                                                        graph.getSampler(colorHnd));
	    });
}
