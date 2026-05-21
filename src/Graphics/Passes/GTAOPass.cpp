#include "GTAOPass.hpp"

#include <Orhescyon/GeneralManager.hpp>

#include "../GraphicsContexts.hpp"
#include "../SwapChain.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Components/PipelineManagerComponent.hpp"
#include "../Components/GraphicsSettingsComponent.hpp"
#include "../Components/GtaoSettingsComponent.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"
#include "../Resources/Managers/Bindings.hpp"
#include "../Managers/PipelineManager.hpp"
#include "../RenderGraph/RenderGraph.hpp"

bool GTAOPass::isEnabled(Orhescyon::GeneralManager& gm) const
{
	return gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>()->enableGtao;
}

void GTAOPass::drawGtao(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, DescriptorManagerComponent& dManager,
                        DSetHandle& gtaoDSet, DSetHandle& globalDSet, const GtaoSettingsComponent& gtaoSettings,
                        PipelineManager& pManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pManager.pipelines["gtao"].pipeline);

	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
	                                static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pManager.pipelines["gtao"].layout, 0,
	                       dManager.descriptorManager->getSet(gtaoDSet), nullptr);

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pManager.pipelines["gtao"].layout, 1,
	                       dManager.descriptorManager->getSet(globalDSet), nullptr);

	struct GtaoPushConstants
	{
		int kernelSize;
		float radius;
		float bias;
		float power;
		int numDirections;
		float maxScreenRadius;
		float fadeStart;
		float fadeEnd;
		float texelSize[2];
	} push;
	push.kernelSize = gtaoSettings.kernelSize;
	push.radius = gtaoSettings.radius;
	push.bias = gtaoSettings.bias;
	push.power = gtaoSettings.power;
	push.numDirections = gtaoSettings.numDirections;
	push.maxScreenRadius = gtaoSettings.maxScreenRadius;
	push.fadeStart = gtaoSettings.fadeStart;
	push.fadeEnd = gtaoSettings.fadeEnd;
	push.texelSize[0] = 1.0f / static_cast<float>(swapChain.swapChainExtent.width);
	push.texelSize[1] = 1.0f / static_cast<float>(swapChain.swapChainExtent.height);

	cmd.pushConstants<GtaoPushConstants>(pManager.pipelines["gtao"].layout, vk::ShaderStageFlagBits::eFragment, 0, push);
	cmd.setCullMode(vk::CullModeFlagBits::eNone);
	cmd.draw(3, 1, 0, 0);
}

void GTAOPass::drawBlur(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, DescriptorManagerComponent& dManager,
                        DSetHandle& blurDSet, float dirX, float dirY, PipelineManager& pManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["gtao_blur"].pipeline);

	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
	                                static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["gtao_blur"].layout, 0,
	                       dManager.descriptorManager->getSet(blurDSet), nullptr);
	struct BlurPushConstants
	{
		float texelSize[2];
		float direction[2];
	} push;
	push.texelSize[0] = 1.0f / static_cast<float>(swapChain.swapChainExtent.width);
	push.texelSize[1] = 1.0f / static_cast<float>(swapChain.swapChainExtent.height);
	push.direction[0] = dirX;
	push.direction[1] = dirY;

	cmd.pushConstants<BlurPushConstants>(*pManager.pipelines["gtao_blur"].layout, vk::ShaderStageFlagBits::eFragment, 0,
	                                     push);
	cmd.setCullMode(vk::CullModeFlagBits::eNone);
	cmd.draw(3, 1, 0, 0);
}

void GTAOPass::drawApply(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, DescriptorManagerComponent& dManager,
                         DSetHandle& applyDSet, PipelineManager& pManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["gtao_apply"].pipeline);

	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
	                                static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["gtao_apply"].layout, 0,
	                       dManager.descriptorManager->getSet(applyDSet), nullptr);

	cmd.setCullMode(vk::CullModeFlagBits::eNone);
	cmd.draw(3, 1, 0, 0);
}

void GTAOPass::addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t /*frame*/)
{
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& globalDSetComponent = *gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& gtaoSettings = *gm.getContextComponent<GtaoSettingsContext, GtaoSettingsComponent>();

	vk::ClearValue clearWhite = vk::ClearColorValue(1.0f, 1.0f, 1.0f, 1.0f);
	vk::ClearValue clearBlack = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);
	rg.addPass(
	    "GTAO",
	    {.colorAttachments = {{"GTAOTexture", vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, clearWhite}}},
	    {{"Depth", RGResourceUsage::ShaderRead},
	     {"ViewNormals", RGResourceUsage::ShaderRead},
	     {"NoiseImage", RGResourceUsage::ShaderRead}},
	    {{"GTAOTexture", RGResourceUsage::ColorAttachmentWrite}},
	    [&](vk::raii::CommandBuffer& cmd)
	    {
		    drawGtao(cmd, swapChain, dManager, globalDSetComponent.gtaoDSets, globalDSetComponent.globalDSets,
		             gtaoSettings, pManager);
	    },
	    [dManager, globalDSetComponent](const RenderGraph& graph, const RGPass& pass)
	    {
		    auto depthHnd = pass.getPhysicalRead("Depth");
		    auto normHnd = pass.getPhysicalRead("ViewNormals");
		    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.gtaoDSets, Bindings::GTAO::DepthInput,
		                                                        graph.getImageView(depthHnd), graph.getSampler(depthHnd));
		    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.gtaoDSets,
		                                                        Bindings::GTAO::NormalsInput, graph.getImageView(normHnd),
		                                                        graph.getSampler(normHnd));
	    });

	rg.addPass(
	    "GTAOBlurH",
	    {.colorAttachments = {{"GTAOTexture", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearWhite}}},
	    {{"GTAOTexture", RGResourceUsage::ShaderRead},
	     {"Depth", RGResourceUsage::ShaderRead},
	     {"ViewNormals", RGResourceUsage::ShaderRead}},
	    {{"GTAOTexture", RGResourceUsage::ColorAttachmentWrite}},
	    [&](vk::raii::CommandBuffer& cmd)
	    {
		    drawBlur(cmd, swapChain, dManager, globalDSetComponent.gtaoBlurHDSets, 1.0f, 0.0f, pManager);
	    },
	    [dManager, globalDSetComponent](const RenderGraph& graph, const RGPass& pass)
	    {
		    auto gtaoHnd = pass.getPhysicalRead("GTAOTexture");
		    auto depthHnd = pass.getPhysicalRead("Depth");
		    auto normHnd = pass.getPhysicalRead("ViewNormals");
		    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.gtaoBlurHDSets,
		                                                        Bindings::GTAOBlur::GtaoInput,
		                                                        graph.getImageView(gtaoHnd), graph.getSampler(gtaoHnd));
		    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.gtaoBlurHDSets,
		                                                        Bindings::GTAOBlur::DepthInput,
		                                                        graph.getImageView(depthHnd), graph.getSampler(depthHnd));
		    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.gtaoBlurHDSets,
		                                                        Bindings::GTAOBlur::NormalsInput,
		                                                        graph.getImageView(normHnd), graph.getSampler(normHnd));
	    });

	rg.addPass(
	    "GTAOBlurV",
	    {.colorAttachments = {{"GTAOTexture", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearWhite}}},
	    {{"GTAOTexture", RGResourceUsage::ShaderRead},
	     {"Depth", RGResourceUsage::ShaderRead},
	     {"ViewNormals", RGResourceUsage::ShaderRead}},
	    {{"GTAOTexture", RGResourceUsage::ColorAttachmentWrite}},
	    [&](vk::raii::CommandBuffer& cmd)
	    {
		    drawBlur(cmd, swapChain, dManager, globalDSetComponent.gtaoBlurVDSets, 0.0f, 1.0f, pManager);
	    },
	    [dManager, globalDSetComponent](const RenderGraph& graph, const RGPass& pass)
	    {
		    auto gtaoHnd = pass.getPhysicalRead("GTAOTexture");
		    auto depthHnd = pass.getPhysicalRead("Depth");
		    auto normHnd = pass.getPhysicalRead("ViewNormals");
		    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.gtaoBlurVDSets,
		                                                        Bindings::GTAOBlur::GtaoInput,
		                                                        graph.getImageView(gtaoHnd), graph.getSampler(gtaoHnd));
		    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.gtaoBlurVDSets,
		                                                        Bindings::GTAOBlur::DepthInput,
		                                                        graph.getImageView(depthHnd), graph.getSampler(depthHnd));
		    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.gtaoBlurVDSets,
		                                                        Bindings::GTAOBlur::NormalsInput,
		                                                        graph.getImageView(normHnd), graph.getSampler(normHnd));
	    });

	rg.addPass(
	    "GTAOApply",
	    {.colorAttachments = {{"MainColor", vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, clearBlack}}},
	    {{"MainColor", RGResourceUsage::ShaderRead}, {"GTAOTexture", RGResourceUsage::ShaderRead}},
	    {{"MainColor", RGResourceUsage::ColorAttachmentWrite}},
	    [&](vk::raii::CommandBuffer& cmd)
	    {
		    drawApply(cmd, swapChain, dManager, globalDSetComponent.gtaoApplyDSets, pManager);
	    },
	    [dManager, globalDSetComponent](const RenderGraph& graph, const RGPass& pass)
	    {
		    auto colorHnd = pass.getPhysicalRead("MainColor");
		    auto blurHnd = pass.getPhysicalRead("GTAOTexture");
		    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.gtaoApplyDSets,
		                                                        Bindings::GTAOApply::ColorInput,
		                                                        graph.getImageView(colorHnd), graph.getSampler(colorHnd));
		    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.gtaoApplyDSets,
		                                                        Bindings::GTAOApply::GtaoInput,
		                                                        graph.getImageView(blurHnd), graph.getSampler(blurHnd));
	    });
}
