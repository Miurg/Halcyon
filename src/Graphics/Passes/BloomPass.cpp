#include "BloomPass.hpp"

#include <Orhescyon/GeneralManager.hpp>

#include "../GraphicsContexts.hpp"
#include "../SwapChain.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Components/PipelineManagerComponent.hpp"
#include "../Components/GraphicsSettingsComponent.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"
#include "../Resources/Managers/Bindings.hpp"
#include "../Managers/PipelineManager.hpp"
#include "../RenderGraph/RenderGraph.hpp"

bool BloomPass::isEnabled(Orhescyon::GeneralManager& gm) const
{
	return gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>()->enableBloom;
}

void BloomPass::drawDownsample(vk::raii::CommandBuffer& cmd, DescriptorManagerComponent& dManager,
                               DSetHandle& dSetHandle, PipelineManager& pManager, float texelSizeX, float texelSizeY,
                               float threshold, float knee, int isFirstPass, vk::Extent2D extent)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["bloom_downsample"].pipeline);

	cmd.setViewport(
	    0, vk::Viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), extent));

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["bloom_downsample"].layout, 0,
	                       dManager.descriptorManager->getSet(dSetHandle), nullptr);

	struct PushConstants
	{
		float texelSize[2];
		float threshold;
		float knee;
		int isFirstPass;
	} push;
	push.texelSize[0] = texelSizeX;
	push.texelSize[1] = texelSizeY;
	push.threshold = threshold;
	push.knee = knee;
	push.isFirstPass = isFirstPass;

	cmd.pushConstants<PushConstants>(*pManager.pipelines["bloom_downsample"].layout, vk::ShaderStageFlagBits::eFragment,
	                                 0, push);
	cmd.setCullMode(vk::CullModeFlagBits::eNone);
	cmd.draw(3, 1, 0, 0);
}

void BloomPass::drawUpsample(vk::raii::CommandBuffer& cmd, DescriptorManagerComponent& dManager,
                             DSetHandle& dSetHandle, PipelineManager& pManager, float texelSizeX, float texelSizeY,
                             float blendFactor, vk::Extent2D extent)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["bloom_upsample"].pipeline);

	cmd.setViewport(
	    0, vk::Viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), extent));

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["bloom_upsample"].layout, 0,
	                       dManager.descriptorManager->getSet(dSetHandle), nullptr);

	struct PushConstants
	{
		float texelSize[2];
		float blendFactor;
	} push;
	push.texelSize[0] = texelSizeX;
	push.texelSize[1] = texelSizeY;
	push.blendFactor = blendFactor;

	cmd.pushConstants<PushConstants>(*pManager.pipelines["bloom_upsample"].layout, vk::ShaderStageFlagBits::eFragment, 0,
	                                 push);
	cmd.setCullMode(vk::CullModeFlagBits::eNone);
	cmd.draw(3, 1, 0, 0);
}

void BloomPass::addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t /*frame*/)
{
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& globalDSetComponent = *gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& graphicsSettings = *gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>();

	vk::ClearValue clearBlack = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);

	constexpr uint32_t kMipCount = 5;

	float threshold = graphicsSettings.bloomThreshold;
	float knee = graphicsSettings.bloomKnee;
	float intensity = graphicsSettings.bloomIntensity;

	auto mipExtent = [&](uint32_t mip)
	{
		uint32_t shift = mip + 1;
		return vk::Extent2D{std::max(1u, swapChain.swapChainExtent.width >> shift),
		                    std::max(1u, swapChain.swapChainExtent.height >> shift)};
	};

	// Downsample
	for (uint32_t i = 0; i < kMipCount; ++i)
	{
		vk::Extent2D dstExt = mipExtent(i);
		vk::Extent2D srcExt = (i == 0) ? swapChain.swapChainExtent : mipExtent(i - 1);
		float texelX = 1.0f / static_cast<float>(srcExt.width);
		float texelY = 1.0f / static_cast<float>(srcExt.height);
		int isFirst = (i == 0) ? 1 : 0;
		uint32_t passIdx = i;

		std::string srcName = (i == 0) ? "MainColor" : "BloomChain";
		uint32_t srcMip = (i == 0) ? 0 : (i - 1);

		RGAttachmentConfig colorAtt{
		    .name = "BloomChain", .loadOp = vk::AttachmentLoadOp::eClear, .storeOp = vk::AttachmentStoreOp::eStore,
		    .clearValue = clearBlack, .mipLevel = i};

		rg.addPass(
		    "BloomDown" + std::to_string(i),
		    {.colorAttachments = {colorAtt}},
		    {{srcName, RGResourceUsage::ShaderRead, srcMip, 1}},
		    {{"BloomChain", RGResourceUsage::ColorAttachmentWrite, i, 1}},
		    [&, texelX, texelY, threshold, knee, isFirst, dstExt, passIdx](vk::raii::CommandBuffer& cmd)
		    {
			    drawDownsample(cmd, dManager, globalDSetComponent.bloomDownsampleDSets[passIdx], pManager, texelX,
			                   texelY, threshold, knee, isFirst, dstExt);
		    },
		    [dManager, globalDSetComponent, passIdx, srcName, srcMip](const RenderGraph& graph, const RGPass& pass)
		    {
			    auto srcHnd = pass.getPhysicalRead(srcName);
			    dManager.descriptorManager->updateSingleTextureDSet(
			        globalDSetComponent.bloomDownsampleDSets[passIdx], Bindings::BloomDownsample::InputTexture,
			        graph.getImageView(srcHnd, srcMip), graph.getSampler(srcHnd));
		    });
	}

	// Upsample 
	for (uint32_t i = 0; i < kMipCount; ++i)
	{
		uint32_t srcMip = kMipCount - 1 - i;
		vk::Extent2D srcExt = mipExtent(srcMip);
		float texelX = 1.0f / static_cast<float>(srcExt.width);
		float texelY = 1.0f / static_cast<float>(srcExt.height);
		uint32_t passIdx = i;

		bool isLast = (i == kMipCount - 1);
		std::string dstName = isLast ? "MainColor" : "BloomChain";
		uint32_t dstMip = isLast ? 0 : (srcMip - 1);
		vk::Extent2D dstExt = isLast ? swapChain.swapChainExtent : mipExtent(dstMip);

		RGAttachmentConfig colorAtt{
		    .name = dstName, .loadOp = vk::AttachmentLoadOp::eLoad, .storeOp = vk::AttachmentStoreOp::eStore,
		    .mipLevel = dstMip};

		rg.addPass(
		    "BloomUp" + std::to_string(i),
		    {.colorAttachments = {colorAtt}},
		    {{"BloomChain", RGResourceUsage::ShaderRead, srcMip, 1}},
		    {{dstName, RGResourceUsage::ColorAttachmentWrite, dstMip, 1}},
		    [&, texelX, texelY, intensity, dstExt, passIdx](vk::raii::CommandBuffer& cmd)
		    {
			    drawUpsample(cmd, dManager, globalDSetComponent.bloomUpsampleDSets[passIdx], pManager, texelX, texelY,
			                 intensity, dstExt);
		    },
		    [dManager, globalDSetComponent, passIdx, srcMip](const RenderGraph& graph, const RGPass& pass)
		    {
			    auto srcHnd = pass.getPhysicalRead("BloomChain");
			    dManager.descriptorManager->updateSingleTextureDSet(
			        globalDSetComponent.bloomUpsampleDSets[passIdx], Bindings::BloomUpsample::CurrentTexture,
			        graph.getImageView(srcHnd, srcMip), graph.getSampler(srcHnd));
		    });
	}
}
