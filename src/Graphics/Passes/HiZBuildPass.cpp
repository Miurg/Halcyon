#include "HiZBuildPass.hpp"

#include <Orhescyon/GeneralManager.hpp>

#include "../GraphicsContexts.hpp"
#include "../SwapChain.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Components/PipelineManagerComponent.hpp"
#include "../Components/RenderGraphComponent.hpp"
#include "../Resources/Managers/DescriptorManager.hpp"
#include "../Managers/PipelineManager.hpp"
#include "../Factories/PipelineFactory.hpp"
#include "../RenderGraph/RenderGraph.hpp"

namespace
{
namespace HiZBinding
{
enum : uint32_t
{
	DepthInput = 0,
	MipOutput = 1,
};
}
} // namespace

void HiZBuildPass::onInit(Orhescyon::GeneralManager& gm)
{
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& rg = *gm.getContextComponent<RenderGraphContext, RenderGraphComponent>()->renderGraph;

	for (uint32_t i = 0; i < kMaxMips; ++i) _dsets[i] = dManager.allocate("hiZSet");

	// Min-reduction sampler - needed so that downsampling preserves the closest (depth-min) value
	static const vk::SamplerReductionModeCreateInfo hiZReductionInfo{vk::SamplerReductionMode::eMin};
	vk::SamplerCreateInfo hiZSamplerInfo{};
	hiZSamplerInfo.pNext = &hiZReductionInfo;
	hiZSamplerInfo.magFilter = vk::Filter::eLinear;
	hiZSamplerInfo.minFilter = vk::Filter::eLinear;
	hiZSamplerInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
	hiZSamplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	hiZSamplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	hiZSamplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
	hiZSamplerInfo.minLod = 0.0f;
	hiZSamplerInfo.maxLod = VK_LOD_CLAMP_NONE;

	rg.declareLogicalStream("HiZ", {.format = vk::Format::eR32Sfloat,
	                                .sizeMode = RGSizeMode::FullExtent,
	                                .aspectFlags = vk::ImageAspectFlagBits::eColor,
	                                .samples = vk::SampleCountFlagBits::e1,
	                                .mipLevels = RG_FULL_MIP_CHAIN,
	                                .extraUsage = vk::ImageUsageFlagBits::eStorage,
	                                .customSamplerInfo = hiZSamplerInfo});

	pManager.build(
	    PipelineDescription{
	        .isCompute = true,
	        .shaderPath = "shaders/hi_z_reduction.spv",
	        .setLayoutNames = {"hiZSet"},
	        .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t) * 3}},
	        // push = { dstWidth, dstHeight, isFirstMip (0/1) }
	    },
	    "hi_z_reduction");
}

void HiZBuildPass::drawDownsample(vk::raii::CommandBuffer& cmd, DescriptorManagerComponent& dManager,
                                  DSetHandle dSetHandle, PipelineManager& pManager, uint32_t dstWidth,
                                  uint32_t dstHeight, uint32_t passIdx)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["hi_z_reduction"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["hi_z_reduction"].layout, 0,
	                       dManager.descriptorManager->getSet(dSetHandle), nullptr);
	struct PushConsts
	{
		uint32_t dstWidth;
		uint32_t dstHeight;
		uint32_t passIdx;
	} push;

	push.dstHeight = dstHeight;
	push.dstWidth = dstWidth;
	push.passIdx = passIdx;

	cmd.pushConstants<PushConsts>(*pManager.pipelines["hi_z_reduction"].layout, vk::ShaderStageFlagBits::eCompute, 0,
	                              push);
	cmd.dispatch((dstWidth + 7) / 8, (dstHeight + 7) / 8, 1);
}

void HiZBuildPass::addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t /*frame*/)
{
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;

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

		std::string srcName = (i == 0) ? "Depth" : "HiZ";
		uint32_t srcMip = (i == 0) ? 0 : (i - 1);

		DSetHandle dset = _dsets[passIdx];

		rg.addPass(
		    "HiZ" + std::to_string(i), {.isCompute = true}, {{srcName, RGResourceUsage::ShaderRead, srcMip, 1}},
		    {{"HiZ", RGResourceUsage::StorageReadWrite, i, 1}},
		    [this, &dManager, &pManager, passIdx, dstExt, dset](vk::raii::CommandBuffer& cmd)
		    { drawDownsample(cmd, dManager, dset, pManager, dstExt.width, dstExt.height, passIdx); },
		    [&dManager, srcName, srcMip, i, dset](const RenderGraph& graph, const RGPass& pass)
		    {
			    auto srcHnd = pass.getPhysicalRead(srcName);
			    auto dstHnd = pass.getPhysicalWrite("HiZ");
			    dManager.descriptorManager->update(
			        dset, HiZBinding::DepthInput, 0, vk::DescriptorType::eCombinedImageSampler,
			        graph.getImageView(srcHnd, srcMip), graph.getSampler(dstHnd), vk::ImageLayout::eShaderReadOnlyOptimal);
			    dManager.descriptorManager->update(dset, HiZBinding::MipOutput, 0, vk::DescriptorType::eStorageImage,
			                                       graph.getImageView(dstHnd, i), vk::Sampler{},
			                                       vk::ImageLayout::eGeneral);
		    });
	}
}
