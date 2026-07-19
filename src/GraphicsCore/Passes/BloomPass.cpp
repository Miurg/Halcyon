#include "BloomPass.hpp"

#include <Orhescyon/GeneralManager.hpp>

#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/SwapChain.hpp"
#include "GraphicsCore/Components/SwapChainComponent.hpp"
#include "GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include "GraphicsCore/Components/PipelineManagerComponent.hpp"
#include "GraphicsCore/Components/GraphicsSettingsComponent.hpp"
#include "GraphicsCore/Components/RenderGraphComponent.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "GraphicsCore/Managers/PipelineManager.hpp"
#include "GraphicsCore/Factories/PipelineFactory.hpp"
#include "GraphicsCore/RenderGraph/RenderGraph.hpp"

namespace
{
namespace DownsampleBinding
{
enum : uint32_t
{
	InputTexture = 0,
};
}
namespace UpsampleBinding
{
enum : uint32_t
{
	CurrentTexture = 0,
};
}
} // namespace

bool BloomPass::isEnabled(Orhescyon::GeneralManager& gm) const
{
	return gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>()->enableBloom;
}

void BloomPass::onInit(Orhescyon::GeneralManager& gm)
{
	auto& descriptorManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	auto& pipelineManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto& rg = *gm.getContextComponent<RenderGraphContext, RenderGraphComponent>()->renderGraph;

	for (uint32_t i = 0; i < kMipCount; ++i)
	{
		_downsampleDsets[i] = descriptorManager.allocate("screenSpaceSet");
		_upsampleDsets[i] = descriptorManager.allocate("screenSpaceSet");
	}

	rg.declareLogicalStream("BloomChain", {swapChain.hdrFormat, RGSizeMode::HalfExtent, vk::ImageAspectFlagBits::eColor,
	                                       vk::SampleCountFlagBits::e1, kMipCount});

	pipelineManager.build(PipelineDescription{
	    .shaderPath = "bloom_downsample.spv",
	    .cullMode = vk::CullModeFlagBits::eNone,
	    .colorAttachments = {PipelineFactory::opaqueAttachment()},
	    .colorFormats = {swapChain.hdrFormat},
	    .setLayoutNames = {"screenSpaceSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eFragment, 0, 20u}},
	});
	pipelineManager.build(PipelineDescription{
	    .shaderPath = "bloom_upsample.spv",
	    .cullMode = vk::CullModeFlagBits::eNone,
	    .colorAttachments = {PipelineFactory::additiveAttachment()},
	    .colorFormats = {swapChain.hdrFormat},
	    .setLayoutNames = {"screenSpaceSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eFragment, 0, 12u}},
	});
}

void BloomPass::drawDownsample(vk::raii::CommandBuffer& cmd, DescriptorManagerComponent& descriptorManager,
                               DSetHandle dSetHandle, PipelineManager& pipelineManager, float texelSizeX, float texelSizeY,
                               float threshold, float knee, int isFirstPass, vk::Extent2D extent)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineManager.pipelines["bloom_downsample"].pipeline);

	cmd.setViewport(
	    0, vk::Viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), extent));

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineManager.pipelines["bloom_downsample"].layout, 0,
	                       descriptorManager.descriptorManager->getSet(dSetHandle), nullptr);

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

	cmd.pushConstants<PushConstants>(*pipelineManager.pipelines["bloom_downsample"].layout, vk::ShaderStageFlagBits::eFragment,
	                                 0, push);
	cmd.setCullMode(vk::CullModeFlagBits::eNone);
	cmd.draw(3, 1, 0, 0);
}

void BloomPass::drawUpsample(vk::raii::CommandBuffer& cmd, DescriptorManagerComponent& descriptorManager, DSetHandle dSetHandle,
                             PipelineManager& pipelineManager, float texelSizeX, float texelSizeY, float blendFactor,
                             vk::Extent2D extent)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineManager.pipelines["bloom_upsample"].pipeline);

	cmd.setViewport(
	    0, vk::Viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), extent));

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineManager.pipelines["bloom_upsample"].layout, 0,
	                       descriptorManager.descriptorManager->getSet(dSetHandle), nullptr);

	struct PushConstants
	{
		float texelSize[2];
		float blendFactor;
	} push;
	push.texelSize[0] = texelSizeX;
	push.texelSize[1] = texelSizeY;
	push.blendFactor = blendFactor;

	cmd.pushConstants<PushConstants>(*pipelineManager.pipelines["bloom_upsample"].layout, vk::ShaderStageFlagBits::eFragment, 0,
	                                 push);
	cmd.setCullMode(vk::CullModeFlagBits::eNone);
	cmd.draw(3, 1, 0, 0);
}

void BloomPass::addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t /*frame*/)
{
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto& descriptorManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& pipelineManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& graphicsSettings = *gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>();

	vk::ClearValue clearBlack = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);

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

		std::string srcName = (i == 0) ? "MainColor" : "BloomChain";
		uint32_t srcMip = (i == 0) ? 0 : (i - 1);

		RGAttachmentConfig colorAtt{.name = "BloomChain",
		                            .loadOp = vk::AttachmentLoadOp::eClear,
		                            .storeOp = vk::AttachmentStoreOp::eStore,
		                            .clearValue = clearBlack,
		                            .mipLevel = i};

		DSetHandle dset = _downsampleDsets[i];

		rg.addPass(
		    "BloomDown" + std::to_string(i), {.colorAttachments = {colorAtt}},
		    {{srcName, RGResourceUsage::ShaderRead, srcMip, 1}},
		    {{"BloomChain", RGResourceUsage::ColorAttachmentWrite, i, 1}},
		    [&, texelX, texelY, threshold, knee, isFirst, dstExt, dset](vk::raii::CommandBuffer& cmd)
		    { drawDownsample(cmd, descriptorManager, dset, pipelineManager, texelX, texelY, threshold, knee, isFirst, dstExt); },
		    [descriptorManager, srcName, srcMip, dset](const RenderGraph& graph, const RGPass& pass)
		    {
			    auto srcHnd = pass.getPhysicalRead(srcName);
			    descriptorManager.descriptorManager->updateSingleTextureDSet(
			        dset, DownsampleBinding::InputTexture, graph.getImageView(srcHnd, srcMip), graph.getSampler(srcHnd));
		    });
	}

	// Upsample
	for (uint32_t i = 0; i < kMipCount; ++i)
	{
		uint32_t srcMip = kMipCount - 1 - i;
		vk::Extent2D srcExt = mipExtent(srcMip);
		float texelX = 1.0f / static_cast<float>(srcExt.width);
		float texelY = 1.0f / static_cast<float>(srcExt.height);

		bool isLast = (i == kMipCount - 1);
		std::string dstName = isLast ? "MainColor" : "BloomChain";
		uint32_t dstMip = isLast ? 0 : (srcMip - 1);
		vk::Extent2D dstExt = isLast ? swapChain.swapChainExtent : mipExtent(dstMip);

		RGAttachmentConfig colorAtt{.name = dstName,
		                            .loadOp = vk::AttachmentLoadOp::eLoad,
		                            .storeOp = vk::AttachmentStoreOp::eStore,
		                            .mipLevel = dstMip};

		DSetHandle dset = _upsampleDsets[i];

		rg.addPass(
		    "BloomUp" + std::to_string(i), {.colorAttachments = {colorAtt}},
		    {{"BloomChain", RGResourceUsage::ShaderRead, srcMip, 1}},
		    {{dstName, RGResourceUsage::ColorAttachmentWrite, dstMip, 1}},
		    [&, texelX, texelY, intensity, dstExt, dset](vk::raii::CommandBuffer& cmd)
		    { drawUpsample(cmd, descriptorManager, dset, pipelineManager, texelX, texelY, intensity, dstExt); },
		    [descriptorManager, srcMip, dset](const RenderGraph& graph, const RGPass& pass)
		    {
			    auto srcHnd = pass.getPhysicalRead("BloomChain");
			    descriptorManager.descriptorManager->updateSingleTextureDSet(
			        dset, UpsampleBinding::CurrentTexture, graph.getImageView(srcHnd, srcMip), graph.getSampler(srcHnd));
		    });
	}
}
