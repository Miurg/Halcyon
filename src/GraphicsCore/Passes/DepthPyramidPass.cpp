#include "DepthPyramidPass.hpp"

#include <Orhescyon/GeneralManager.hpp>

#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/SwapChain.hpp"
#include "GraphicsCore/Components/SwapChainComponent.hpp"
#include "GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include "GraphicsCore/Components/PipelineManagerComponent.hpp"
#include "GraphicsCore/Components/RenderGraphComponent.hpp"
#include "GraphicsCore/Components/GtaoSettingsComponent.hpp"
#include "GraphicsCore/Resources/Components/GlobalDSetComponent.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "GraphicsCore/Managers/PipelineManager.hpp"
#include "GraphicsCore/Factories/PipelineFactory.hpp"
#include "GraphicsCore/RenderGraph/RenderGraph.hpp"

namespace
{
namespace DepthPyramidBinding
{
enum : uint32_t
{
	DepthInput = 0,
	MipOutput = 1,
};
}
} // namespace

void DepthPyramidPass::onInit(Orhescyon::GeneralManager& gm)
{
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& rg = *gm.getContextComponent<RenderGraphContext, RenderGraphComponent>()->renderGraph;

	for (uint32_t i = 0; i < kMaxMips; ++i) _dsets[i] = dManager.allocate("hiZSet");

	// Linear view-space Z pyramid sampling
	SamplerDesc pyramidSampler;
	pyramidSampler.mipmapMode = SamplerMipmapMode::Nearest;
	pyramidSampler.addressMode = SamplerAddressMode::ClampToEdge;

	rg.declareLogicalStream("DepthPyramid", {.format = vk::Format::eR32Sfloat,
	                                         .sizeMode = RGSizeMode::FullExtent,
	                                         .aspectFlags = vk::ImageAspectFlagBits::eColor,
	                                         .samples = vk::SampleCountFlagBits::e1,
	                                         .mipLevels = RG_FULL_MIP_CHAIN,
	                                         .extraUsage = vk::ImageUsageFlagBits::eStorage,
	                                         .samplerOverride = pyramidSampler});

	pManager.build(
	    PipelineDescription{
	        .isCompute = true,
	        .shaderPath = "depth_pyramid.spv",
	        .setLayoutNames = {"hiZSet", "globalSet"},
	        .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t) * 3 + sizeof(float)}},
	        // push = { dstWidth, dstHeight, passIdx, edgeRange }
	    },
	    "depth_pyramid");
}

void DepthPyramidPass::drawDownsample(vk::raii::CommandBuffer& cmd, DescriptorManagerComponent& dManager,
                                      DSetHandle dSetHandle, DSetHandle globalDSet, PipelineManager& pManager,
                                      uint32_t dstWidth, uint32_t dstHeight, uint32_t passIdx, float edgeRange)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["depth_pyramid"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["depth_pyramid"].layout, 0,
	                       dManager.descriptorManager->getSet(dSetHandle), nullptr);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["depth_pyramid"].layout, 1,
	                       dManager.descriptorManager->getSet(globalDSet), nullptr);
	struct PushConsts
	{
		uint32_t dstWidth;
		uint32_t dstHeight;
		uint32_t passIdx;
		float edgeRange;
	} push;

	push.dstHeight = dstHeight;
	push.dstWidth = dstWidth;
	push.passIdx = passIdx;
	push.edgeRange = edgeRange;

	cmd.pushConstants<PushConsts>(*pManager.pipelines["depth_pyramid"].layout, vk::ShaderStageFlagBits::eCompute, 0,
	                              push);
	cmd.dispatch((dstWidth + 7) / 8, (dstHeight + 7) / 8, 1);
}

void DepthPyramidPass::addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t /*frame*/)
{
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& globalDSetComponent = *gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	auto& gtaoSettings = *gm.getContextComponent<GtaoSettingsContext, GtaoSettingsComponent>();

	const uint32_t kMipCount =
	    std::bit_width(std::max(swapChain.swapChainExtent.width, swapChain.swapChainExtent.height));

	auto mipExtent = [&](uint32_t mip)
	{
		uint32_t shift = mip;
		return vk::Extent2D{std::max(1u, swapChain.swapChainExtent.width >> shift),
		                    std::max(1u, swapChain.swapChainExtent.height >> shift)};
	};

	for (uint32_t i = 0; i < kMipCount; ++i)
	{
		vk::Extent2D dstExt = mipExtent(i);
		uint32_t passIdx = i;

		std::string srcName = (i == 0) ? "Depth" : "DepthPyramid";
		uint32_t srcMip = (i == 0) ? 0 : (i - 1);

		DSetHandle dset = _dsets[passIdx];

		rg.addPass(
		    "DepthPyramid" + std::to_string(i), {.isCompute = true},
		    {{srcName, RGResourceUsage::ShaderRead, srcMip, 1}},
		    {{"DepthPyramid", RGResourceUsage::StorageReadWrite, i, 1}},
		    [this, &dManager, &pManager, &gtaoSettings, passIdx, dstExt, dset,
		     globalDSet = globalDSetComponent.globalDSets](vk::raii::CommandBuffer& cmd) {
			    drawDownsample(cmd, dManager, dset, globalDSet, pManager, dstExt.width, dstExt.height, passIdx,
			                   gtaoSettings.pyramidEdgeRange);
		    },
		    [&dManager, srcName, srcMip, i, dset](const RenderGraph& graph, const RGPass& pass)
		    {
			    auto srcHnd = pass.getPhysicalRead(srcName);
			    auto dstHnd = pass.getPhysicalWrite("DepthPyramid");
			    dManager.descriptorManager->update(
			        dset, DepthPyramidBinding::DepthInput, 0, vk::DescriptorType::eCombinedImageSampler,
			        graph.getImageView(srcHnd, srcMip), graph.getSampler(dstHnd), vk::ImageLayout::eShaderReadOnlyOptimal);
			    dManager.descriptorManager->update(dset, DepthPyramidBinding::MipOutput, 0,
			                                       vk::DescriptorType::eStorageImage, graph.getImageView(dstHnd, i),
			                                       vk::Sampler{}, vk::ImageLayout::eGeneral);
		    });
	}
}
