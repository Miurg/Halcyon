#include "ToneMappingPass.hpp"

#include <Orhescyon/GeneralManager.hpp>

#include "../GraphicsContexts.hpp"
#include "../SwapChain.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Components/PipelineManagerComponent.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"
#include "../Resources/Managers/Bindings.hpp"
#include "../Managers/PipelineManager.hpp"
#include "../RenderGraph/RenderGraph.hpp"

void ToneMappingPass::addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t /*frame*/)
{
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& globalDSetComponent = *gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;

	vk::ClearValue clearBlack = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);
	rg.addPass(
	    "ToneMapping",
	    {.colorAttachments = {{"PostProcessColor", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
	                           clearBlack}}},
	    {{"MainColor", RGResourceUsage::ShaderRead}}, {{"PostProcessColor", RGResourceUsage::ColorAttachmentWrite}},
	    [&](vk::raii::CommandBuffer& cmd)
	    {
		    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["tone_mapping"].pipeline);

		    cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
		                                    static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
		    cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

		    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["tone_mapping"].layout, 0,
		                           dManager.descriptorManager->getSet(globalDSetComponent.toneMappingDSets), nullptr);

		    cmd.setCullMode(vk::CullModeFlagBits::eNone);
		    cmd.draw(3, 1, 0, 0);
	    },
	    [dManager, globalDSetComponent](const RenderGraph& graph, const RGPass& pass)
	    {
		    auto colorHnd = pass.getPhysicalRead("MainColor");
		    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.toneMappingDSets,
		                                                        Bindings::ToneMapping::OffscreenInput,
		                                                        graph.getImageView(colorHnd), graph.getSampler(colorHnd));
	    });
}
