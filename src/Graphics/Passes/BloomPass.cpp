#include "RenderPasses.hpp"

void drawBloomDownsamplePass(vk::raii::CommandBuffer& cmd, DescriptorManagerComponent& dManager, DSetHandle& dSetHandle,
                             PipelineManager& pManager, float texelSizeX, float texelSizeY, float threshold, float knee,
                             int isFirstPass, vk::Extent2D extent)
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

void drawBloomUpsamplePass(vk::raii::CommandBuffer& cmd, DescriptorManagerComponent& dManager, DSetHandle& dSetHandle,
                           PipelineManager& pManager, float texelSizeX, float texelSizeY, float blendFactor,
                           int isLastPass, vk::Extent2D extent)
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
		int isLastPass;
	} push;
	push.texelSize[0] = texelSizeX;
	push.texelSize[1] = texelSizeY;
	push.blendFactor = blendFactor;
	push.isLastPass = isLastPass;

	cmd.pushConstants<PushConstants>(*pManager.pipelines["bloom_upsample"].layout, vk::ShaderStageFlagBits::eFragment, 0,
	                                 push);
	cmd.setCullMode(vk::CullModeFlagBits::eNone);
	cmd.draw(3, 1, 0, 0);
}

void RenderPasses::BloomPass(SwapChain& swapChain, DescriptorManagerComponent& dManager,
                             GlobalDSetComponent& globalDSetComponent, PipelineManager& pManager, RenderGraph& rg,
                             GraphicsSettingsComponent& graphicsSettings)
{
	vk::ClearValue clearBlack = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);
	const std::string mipNames[5] = {"BloomMip0", "BloomMip1", "BloomMip2", "BloomMip3", "BloomMip4"};
	const std::string downSource[5] = {"MainColor", "BloomMip0", "BloomMip1", "BloomMip2", "BloomMip3"};

	// Divisors matching the RGSizeMode of each mip level
	const uint32_t divisors[5] = {2, 4, 8, 16, 32};

	float threshold = graphicsSettings.bloomThreshold;
	float knee = graphicsSettings.bloomKnee;
	float intensity = graphicsSettings.bloomIntensity;

	// Downsample
	for (int i = 0; i < 5; i++)
	{
		uint32_t sourceDivision = (i == 0) ? 1 : divisors[i - 1];
		uint32_t dstDivision = divisors[i];
		float srcW = static_cast<float>(swapChain.swapChainExtent.width / sourceDivision);
		float srcH = static_cast<float>(swapChain.swapChainExtent.height / sourceDivision);
		uint32_t dstW = swapChain.swapChainExtent.width / dstDivision;
		uint32_t dstH = swapChain.swapChainExtent.height / dstDivision;
		float texelX = 1.0f / srcW;
		float texelY = 1.0f / srcH;
		int isFirst = (i == 0) ? 1 : 0;
		int passIdx = i;

		rg.addPass(
		    "BloomDown" + std::to_string(i),
		    {.colorAttachments = {{mipNames[i], vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
		                           clearBlack}}},
		    {{downSource[i], RGResourceUsage::ShaderRead}}, {{mipNames[i], RGResourceUsage::ColorAttachmentWrite}},
		    [&, texelX, texelY, threshold, knee, isFirst, dstW, dstH, passIdx](vk::raii::CommandBuffer& cmd)
		    {
			    drawBloomDownsamplePass(cmd, dManager, globalDSetComponent.bloomDownsampleDSets[passIdx], pManager, texelX,
			                            texelY, threshold, knee, isFirst, vk::Extent2D{dstW, dstH});
		    },
		    [dManager, globalDSetComponent, passIdx, srcName = downSource[i]](const RenderGraph& graph,
		                                                                      const RGPass& pass)
		    {
			    auto srcHnd = pass.getPhysicalRead(srcName);
			    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.bloomDownsampleDSets[passIdx],
			                                                        Bindings::BloomDownsample::InputTexture,
			                                                        graph.getImageView(srcHnd), graph.getSampler(srcHnd));
		    });
	}

	// Upsample
	const std::string upCurrentSrc[5] = {"BloomMip4", "BloomMip3", "BloomMip2", "BloomMip1", "BloomMip0"};
	const std::string upPrevDst[5] = {"BloomMip3", "BloomMip2", "BloomMip1", "BloomMip0", "MainColor"};

	// Divisors for the destination (previous / target)
	const uint32_t upDstDivisors[5] = {16, 8, 4, 2, 1};

	for (int i = 0; i < 5; i++)
	{
		uint32_t currentDivision = (i == 0) ? 32 : upDstDivisors[i - 1]; // current = source being upsampled
		uint32_t dstDivision = upDstDivisors[i];
		uint32_t dstW = swapChain.swapChainExtent.width / dstDivision;
		uint32_t dstH = swapChain.swapChainExtent.height / dstDivision;
		// Texel size of the CURRENT (lower-res source being upsampled)
		float curW = static_cast<float>(swapChain.swapChainExtent.width / currentDivision);
		float curH = static_cast<float>(swapChain.swapChainExtent.height / currentDivision);
		float texelX = 1.0f / curW;
		float texelY = 1.0f / curH;
		int isLast = (i == 4) ? 1 : 0;
		int passIdx = i;

		rg.addPass(
		    "BloomUp" + std::to_string(i),
		    {.colorAttachments = {{upPrevDst[i], vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore}}},
		    {{upCurrentSrc[i], RGResourceUsage::ShaderRead}, {upPrevDst[i], RGResourceUsage::ShaderRead}},
		    {{upPrevDst[i], RGResourceUsage::ColorAttachmentWrite}},
		    [&, texelX, texelY, intensity, isLast, dstW, dstH, passIdx](vk::raii::CommandBuffer& cmd)
		    {
			    drawBloomUpsamplePass(cmd, dManager, globalDSetComponent.bloomUpsampleDSets[passIdx], pManager, texelX,
			                          texelY, intensity, isLast, vk::Extent2D{dstW, dstH});
		    },
		    [dManager, globalDSetComponent, passIdx, curName = upCurrentSrc[i],
		     prevName = upPrevDst[i]](const RenderGraph& graph, const RGPass& pass)
		    {
			    auto curHnd = pass.getPhysicalRead(curName);
			    auto prevHnd = pass.getPhysicalRead(prevName);
			    dManager.descriptorManager->updateSingleTextureDSet(globalDSetComponent.bloomUpsampleDSets[passIdx],
			                                                        Bindings::BloomUpsample::CurrentTexture,
			                                                        graph.getImageView(curHnd), graph.getSampler(curHnd));
			    dManager.descriptorManager->updateSingleTextureDSet(
			        globalDSetComponent.bloomUpsampleDSets[passIdx], Bindings::BloomUpsample::PreviousTexture,
			        graph.getImageView(prevHnd), graph.getSampler(prevHnd));
		    });
	}
}