#include "RenderPasses.hpp"

void drawSsaoPass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain,
                                        DescriptorManagerComponent& dManager, DSetHandle& ssaoDescriptorSetIndex,
                                        DSetHandle& globalDescriptorSetIndex, const SsaoSettingsComponent& ssaoSettings,
                                        uint32_t frameNumber, PipelineManager& pManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pManager.pipelines["ssao"].pipeline);

	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width / 2.0),
	                                static_cast<float>(swapChain.swapChainExtent.height / 2.0), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D{swapChain.swapChainExtent.width / 2,
	                                                              swapChain.swapChainExtent.height / 2}));

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pManager.pipelines["ssao"].layout, 0,
	                       dManager.descriptorManager->getSet(ssaoDescriptorSetIndex), nullptr);

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pManager.pipelines["ssao"].layout, 1,
	                       dManager.descriptorManager->getSet(globalDescriptorSetIndex), nullptr);

	struct SsaoPushConstants
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
		uint32_t frameNumber;
	} push;
	push.kernelSize = ssaoSettings.kernelSize;
	push.radius = ssaoSettings.radius;
	push.bias = ssaoSettings.bias;
	push.power = ssaoSettings.power;
	push.numDirections = ssaoSettings.numDirections;
	push.maxScreenRadius = ssaoSettings.maxScreenRadius;
	push.fadeStart = ssaoSettings.fadeStart;
	push.fadeEnd = ssaoSettings.fadeEnd;
	push.texelSize[0] = 1.0f / static_cast<float>(swapChain.swapChainExtent.width / 2.0);
	push.texelSize[1] = 1.0f / static_cast<float>(swapChain.swapChainExtent.height / 2.0);
	push.frameNumber = frameNumber;

	cmd.pushConstants<SsaoPushConstants>(pManager.pipelines["ssao"].layout, vk::ShaderStageFlagBits::eFragment, 0, push);
	cmd.setCullMode(vk::CullModeFlagBits::eNone);
	cmd.draw(3, 1, 0, 0);
}

void drawSsaoBlurPass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain,
                                            DescriptorManagerComponent& dManager,
                                            DSetHandle& ssaoBlurDescriptorSetIndex, float dirX, float dirY,
                                            PipelineManager& pManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["ssao_blur"].pipeline);

	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width / 2.0),
	                                static_cast<float>(swapChain.swapChainExtent.height / 2.0), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D{swapChain.swapChainExtent.width / 2,
	                                                              swapChain.swapChainExtent.height / 2}));

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["ssao_blur"].layout, 0,
	                       dManager.descriptorManager->getSet(ssaoBlurDescriptorSetIndex), nullptr);
	struct BlurPushConstants
	{
		float texelSize[2];
		float direction[2];
	} push;
	push.texelSize[0] = 1.0f / static_cast<float>(swapChain.swapChainExtent.width / 2.0);
	push.texelSize[1] = 1.0f / static_cast<float>(swapChain.swapChainExtent.height / 2.0);
	push.direction[0] = dirX;
	push.direction[1] = dirY;

	cmd.pushConstants<BlurPushConstants>(*pManager.pipelines["ssao_blur"].layout, vk::ShaderStageFlagBits::eFragment, 0,
	                                     push);
	cmd.setCullMode(vk::CullModeFlagBits::eNone);
	cmd.draw(3, 1, 0, 0);
}

void drawSSAOApplyPass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain,
                                             DescriptorManagerComponent& dManager,
                                             DSetHandle& ssaoApplyDescriptorSetIndex, PipelineManager& pManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["ssao_apply"].pipeline);

	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
	                                static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["ssao_apply"].layout, 0,
	                       dManager.descriptorManager->getSet(ssaoApplyDescriptorSetIndex), nullptr);

	cmd.setCullMode(vk::CullModeFlagBits::eNone);
	cmd.draw(3, 1, 0, 0);
}

void RenderPasses::SSAOPass(SwapChain& swapChain, CurrentFrameComponent& currentFrameComp,
                            BindlessTextureDSetComponent& bindlessTextureDSetComponent,
                            DescriptorManagerComponent& dManager, GlobalDSetComponent& globalDSetComponent,
                            PipelineManager& pManager, RenderGraph& rg, SsaoSettingsComponent& ssaoSettings)
{
	vk::ClearValue clearWhite = vk::ClearColorValue(1.0f, 1.0f, 1.0f, 1.0f);
	vk::ClearValue clearBlack = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);
	rg.addPass(
	    "SSAO",
	    {.colorAttachments = {{"SSAOTexture", vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, clearWhite}}},
	    {{"Depth", RGResourceUsage::ShaderRead},
	     {"ViewNormals", RGResourceUsage::ShaderRead},
	     {"NoiseImage", RGResourceUsage::ShaderRead}},
	    {{"SSAOTexture", RGResourceUsage::ColorAttachmentWrite}},
	    [&](vk::raii::CommandBuffer& cmd)
	    {
		    drawSsaoPass(cmd, swapChain, dManager, globalDSetComponent.ssaoDSets,
		                                       globalDSetComponent.globalDSets, ssaoSettings,
		                                       currentFrameComp.frameNumber, pManager);
	    },
	    [dManager, globalDSetComponent](const RenderGraph& graph, const RGPass& pass)
	    {
		    auto depthHnd = pass.getPhysicalRead("Depth");
		    auto normHnd = pass.getPhysicalRead("ViewNormals");
		    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.ssaoDSets, Bindings::SSAO::DepthInput,
		                                                        graph.getImageView(depthHnd), graph.getSampler(depthHnd));
		    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.ssaoDSets,
		                                                        Bindings::SSAO::NormalsInput, graph.getImageView(normHnd),
		                                                        graph.getSampler(normHnd));
	    });

	rg.addPass(
	    "SSAOBlurH",
	    {.colorAttachments = {{"SSAOTexture", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearWhite}}},
	    {{"SSAOTexture", RGResourceUsage::ShaderRead},
	     {"Depth", RGResourceUsage::ShaderRead},
	     {"ViewNormals", RGResourceUsage::ShaderRead}},
	    {{"SSAOTexture", RGResourceUsage::ColorAttachmentWrite}},
	    [&](vk::raii::CommandBuffer& cmd)
	    {
		    drawSsaoBlurPass(cmd, swapChain, dManager, globalDSetComponent.ssaoBlurHDSets, 1.0f,
		                                           0.0f, pManager);
	    },
	    [dManager, globalDSetComponent](const RenderGraph& graph, const RGPass& pass)
	    {
		    auto ssaoHnd = pass.getPhysicalRead("SSAOTexture");
		    auto depthHnd = pass.getPhysicalRead("Depth");
		    auto normHnd = pass.getPhysicalRead("ViewNormals");
		    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.ssaoBlurHDSets,
		                                                        Bindings::SSAOBlur::SsaoInput,
		                                                        graph.getImageView(ssaoHnd), graph.getSampler(ssaoHnd));
		    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.ssaoBlurHDSets,
		                                                        Bindings::SSAOBlur::DepthInput,
		                                                        graph.getImageView(depthHnd), graph.getSampler(depthHnd));
		    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.ssaoBlurHDSets,
		                                                        Bindings::SSAOBlur::NormalsInput,
		                                                        graph.getImageView(normHnd), graph.getSampler(normHnd));
	    });

	rg.addPass(
	    "SSAOBlurV",
	    {.colorAttachments = {{"SSAOTexture", vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearWhite}}},
	    {{"SSAOTexture", RGResourceUsage::ShaderRead},
	     {"Depth", RGResourceUsage::ShaderRead},
	     {"ViewNormals", RGResourceUsage::ShaderRead}},
	    {{"SSAOTexture", RGResourceUsage::ColorAttachmentWrite}},
	    [&](vk::raii::CommandBuffer& cmd)
	    {
		    drawSsaoBlurPass(cmd, swapChain, dManager, globalDSetComponent.ssaoBlurVDSets, 0.0f,
		                                           1.0f, pManager);
	    },
	    [dManager, globalDSetComponent](const RenderGraph& graph, const RGPass& pass)
	    {
		    auto ssaoHnd = pass.getPhysicalRead("SSAOTexture");
		    auto depthHnd = pass.getPhysicalRead("Depth");
		    auto normHnd = pass.getPhysicalRead("ViewNormals");
		    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.ssaoBlurVDSets,
		                                                        Bindings::SSAOBlur::SsaoInput,
		                                                        graph.getImageView(ssaoHnd), graph.getSampler(ssaoHnd));
		    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.ssaoBlurVDSets,
		                                                        Bindings::SSAOBlur::DepthInput,
		                                                        graph.getImageView(depthHnd), graph.getSampler(depthHnd));
		    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.ssaoBlurVDSets,
		                                                        Bindings::SSAOBlur::NormalsInput,
		                                                        graph.getImageView(normHnd), graph.getSampler(normHnd));
	    });

	rg.addPass(
	    "SSAOApply",
	    {.colorAttachments = {{"MainColor", vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, clearBlack}}},
	    {{"MainColor", RGResourceUsage::ShaderRead}, {"SSAOTexture", RGResourceUsage::ShaderRead}},
	    {{"MainColor", RGResourceUsage::ColorAttachmentWrite}}, // Overwrites MainColor!
	    [&](vk::raii::CommandBuffer& cmd)
	    {
		    drawSSAOApplyPass(cmd, swapChain, dManager, globalDSetComponent.ssaoApplyDSets,
		                                            pManager);
	    },
	    [dManager, globalDSetComponent](const RenderGraph& graph, const RGPass& pass)
	    {
		    auto colorHnd = pass.getPhysicalRead("MainColor");
		    auto blurHnd = pass.getPhysicalRead("SSAOTexture");
		    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.ssaoApplyDSets,
		                                                        Bindings::SSAOApply::ColorInput,
		                                                        graph.getImageView(colorHnd), graph.getSampler(colorHnd));
		    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.ssaoApplyDSets,
		                                                        Bindings::SSAOApply::SsaoInput,
		                                                        graph.getImageView(blurHnd), graph.getSampler(blurHnd));
	    });
}