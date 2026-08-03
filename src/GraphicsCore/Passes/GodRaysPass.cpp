#include "GodRaysPass.hpp"

#include <Orhescyon/GeneralManager.hpp>

#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/SwapChain.hpp"
#include "GraphicsCore/Components/SwapChainComponent.hpp"
#include "GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include "GraphicsCore/Components/PipelineManagerComponent.hpp"
#include "GraphicsCore/Components/GraphicsSettingsComponent.hpp"
#include "GraphicsCore/Components/GodRaysSettingsComponent.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "GraphicsCore/Managers/PipelineManager.hpp"
#include "GraphicsCore/Factories/PipelineFactory.hpp"
#include "GraphicsCore/RenderGraph/RenderGraph.hpp"
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/GlobalDSetComponent.hpp"
namespace
{
enum Binding : uint32_t
{
	ColorInput = 0,
	DepthInput = 1
};
}

bool GodRaysPass::isEnabled(Orhescyon::GeneralManager& gm) const
{
    return gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>()->enableGodRays;
}

void GodRaysPass::onInit(Orhescyon::GeneralManager& gm)
{
	auto& descriptorManager =
	    *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	auto& pipelineManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;

	_dset = descriptorManager.allocate("screenSpaceSet", MAX_FRAMES_IN_FLIGHT);

	pipelineManager.build(PipelineDescription{
	    .shaderPath = "god_rays.spv",
	    .cullMode = vk::CullModeFlagBits::eNone,
	    .colorAttachments = {PipelineFactory::opaqueAttachment()},
	    .colorFormats = {swapChain.hdrFormat},
	    .setLayoutNames = {"screenSpaceSet", "globalSet", "textureSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eFragment, 0, sizeof(uint32_t) + sizeof(float) * 3}},
	});
}

void GodRaysPass::addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame)
{
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto& descriptorManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& pipelineManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& bindlessTextureDSetComponent = *gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
	auto& globalDSetComponent = *gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
    auto& godRaysSettings = *gm.getContextComponent<GodRaysSettingsContext, GodRaysSettingsComponent>();

	vk::ClearValue clearBlack = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);
	rg.addPass(
	    "GodRays",
	    {.colorAttachments = {{"MainColor", vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
	                           clearBlack}}},
	    {{"MainColor", RGResourceUsage::ShaderRead}, {"Depth", RGResourceUsage::ShaderRead}},
	    {{"MainColor", RGResourceUsage::ColorAttachmentWrite}},
	    [&, dset = _dset, frame](vk::raii::CommandBuffer& cmd)
	    {
		    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineManager.pipelines["god_rays"].pipeline);

		    cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
		                                    static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
		    cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

		    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineManager.pipelines["god_rays"].layout, 0,
		                           descriptorManager.descriptorManager->getSet(dset, frame), nullptr);
		    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineManager.pipelines["god_rays"].layout, 1,
		                           descriptorManager.descriptorManager->getSet(globalDSetComponent.globalDSets, frame),
		                           nullptr);
		    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineManager.pipelines["god_rays"].layout, 2,
		                           descriptorManager.descriptorManager->getSet(bindlessTextureDSetComponent.bindlessTextureSet),
		                           nullptr);

		    struct GodRaysPushConstants
            {
                uint32_t raymarchStepCount;
                float extinctionCoefficient;
                float scatteringCoefficient;
                float fogDensity;
            } push{
                static_cast<uint32_t>(godRaysSettings.raymarchStepCount), godRaysSettings.extinctionCoefficient,
                godRaysSettings.scatteringCoefficient, godRaysSettings.fogDensity};
            cmd.pushConstants<GodRaysPushConstants>(*pipelineManager.pipelines["god_rays"].layout,
                                                     vk::ShaderStageFlagBits::eFragment, 0, push);




cmd.setCullMode(vk::CullModeFlagBits::eNone);
		    cmd.draw(3, 1, 0, 0);
	    },
	    [&descriptorManager, dset = _dset](const RenderGraph& graph, const RGPass& pass)
	    {
		    auto colorHnd = pass.getPhysicalRead("MainColor");
		    descriptorManager.descriptorManager->updateSingleTextureDSet(
		        dset, Binding::ColorInput, graph.getImageView(colorHnd), graph.getSampler(colorHnd));
		    auto depthHnd = pass.getPhysicalRead("Depth");
		    descriptorManager.descriptorManager->updateSingleTextureDSet(
		        dset, Binding::DepthInput, graph.getImageView(depthHnd), graph.getSampler(depthHnd));
	    });
}
