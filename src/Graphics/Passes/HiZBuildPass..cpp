#include "HiZBuildPass.hpp"

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
#include "../VulkanDevice.hpp"
#include "../Components/VulkanDeviceComponent.hpp"

void HiZBuildPass::drawDownsample(vk::raii::CommandBuffer& cmd, DescriptorManagerComponent& dManager,
                                  DSetHandle& dSetHandle, PipelineManager& pManager, uint32_t dstWidth,
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
	auto& globalDSetComponent = *gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& graphicsSettings = *gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>();

	vk::ClearValue clearBlack = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);

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
		vk::Extent2D srcExt = (i == 0) ? swapChain.swapChainExtent : mipExtent(i - 1);
		uint32_t passIdx = i;

		std::string srcName = (i == 0) ? "Depth" : "HiZ";
		uint32_t isFirstMip = (i == 0) ? 1 : 0;
		uint32_t srcMip = (i == 0) ? 0 : (i - 1);

		RGAttachmentConfig colorAtt{.name = "HiZ",
		                            .loadOp = vk::AttachmentLoadOp::eClear,
		                            .storeOp = vk::AttachmentStoreOp::eStore,
		                            .clearValue = clearBlack,
		                            .mipLevel = i};

		rg.addPass(
		    "HiZ" + std::to_string(i), {.isCompute = true}, {{srcName, RGResourceUsage::ShaderRead, srcMip, 1}},
		    {{"HiZ", RGResourceUsage::StorageReadWrite, i, 1}},
		    [this, &dManager, &globalDSetComponent, &pManager, passIdx, srcExt, dstExt](vk::raii::CommandBuffer& cmd)
		    {
			    drawDownsample(cmd, dManager, globalDSetComponent.HiZDSets[passIdx], pManager, dstExt.width, dstExt.height,
			                   passIdx);
		    },
		    [&dManager, &globalDSetComponent, passIdx, srcName, srcMip, i](const RenderGraph& graph, const RGPass& pass)
		    {
			    auto srcHnd = pass.getPhysicalRead(srcName);
			    auto dstHnd = pass.getPhysicalWrite("HiZ");
			    dManager.descriptorManager->update(
			        globalDSetComponent.HiZDSets[passIdx], Bindings::HiZ::DepthInput, 0,
			        vk::DescriptorType::eCombinedImageSampler, graph.getImageView(srcHnd, srcMip),
			        graph.getSampler(dstHnd), vk::ImageLayout::eShaderReadOnlyOptimal);
			    dManager.descriptorManager->update(
			        globalDSetComponent.HiZDSets[passIdx], Bindings::HiZ::MipOutput, 0,
			        vk::DescriptorType::eStorageImage, graph.getImageView(dstHnd, i), vk::Sampler{},
			        vk::ImageLayout::eGeneral);
		    });
	}
}
