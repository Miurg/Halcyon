#include "ClusteredComputePass.hpp"

#include <Orhescyon/GeneralManager.hpp>
#include <GraphicsCore/Components/BufferManagerComponent.hpp>

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

void ClusteredComputePass::computeClustered(vk::raii::CommandBuffer& cmd, uint32_t frame,
                                            DescriptorManagerComponent& descriptorManager, DSetHandle globalDSet,
                                            PipelineManager& pipelineManager, uint32_t widthScreen,
                                            uint32_t heightScreen, vk::Buffer clusteredGridBuffer,
                                            vk::Buffer clusteredInfoBuffer)
{
	cmd.fillBuffer(clusteredInfoBuffer, 0, sizeof(uint32_t) * 2, 0u);

	vk::BufferMemoryBarrier2 resetBarrier;
	resetBarrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
	resetBarrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
	resetBarrier.dstStageMask = vk::PipelineStageFlagBits2::eComputeShader;
	resetBarrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite;
	resetBarrier.buffer = clusteredInfoBuffer;
	resetBarrier.offset = 0;
	resetBarrier.size = sizeof(uint32_t) * 2;

	vk::DependencyInfo resetDependency;
	resetDependency.bufferMemoryBarrierCount = 1;
	resetDependency.pBufferMemoryBarriers = &resetBarrier;
	cmd.pipelineBarrier2(resetDependency);

	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pipelineManager.pipelines["clustered_compute"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pipelineManager.pipelines["clustered_compute"].layout, 0,
	                       descriptorManager.descriptorManager->getSet(globalDSet, frame), nullptr);
	struct PushConsts
	{
		uint32_t widthScreen;
		uint32_t heightScreen;
	} push;

	push.widthScreen = widthScreen;
	push.heightScreen = heightScreen;

	cmd.pushConstants<PushConsts>(*pipelineManager.pipelines["clustered_compute"].layout,
	                              vk::ShaderStageFlagBits::eCompute, 0, push);
	cmd.dispatch((widthScreen + TILE_SIZE - 1) / TILE_SIZE, (heightScreen + TILE_SIZE - 1) / TILE_SIZE, Z_SLICES);

	vk::BufferMemoryBarrier2 clusteredReadBarriers[2];
	for (vk::BufferMemoryBarrier2& barrier : clusteredReadBarriers)
	{
		barrier.srcStageMask = vk::PipelineStageFlagBits2::eComputeShader;
		barrier.srcAccessMask = vk::AccessFlagBits2::eShaderWrite;
		barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
		barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
		barrier.offset = 0;
		barrier.size = VK_WHOLE_SIZE;
	}
	clusteredReadBarriers[0].buffer = clusteredGridBuffer;
	clusteredReadBarriers[1].buffer = clusteredInfoBuffer;

	vk::DependencyInfo clusteredReadDependency;
	clusteredReadDependency.bufferMemoryBarrierCount = 2;
	clusteredReadDependency.pBufferMemoryBarriers = clusteredReadBarriers;
	cmd.pipelineBarrier2(clusteredReadDependency);
}

void ClusteredComputePass::onInit(Orhescyon::GeneralManager& gm)
{
	auto& pipelineManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;

	pipelineManager.build(PipelineDescription{
	    .isCompute = true,
	    .shaderPath = "clustered_compute.spv",
	    .setLayoutNames = {"globalSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t) * 2}},
	});
}

void ClusteredComputePass::addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame)
{
	auto& descriptorManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& globalDSetComponent = *gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	auto& bufferManager = *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	auto& pipelineManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;

	rg.addPass("ComputeClustered", {.isCompute = true}, {}, {},
	           [&, frame](vk::raii::CommandBuffer& cmd)
	           {
		           computeClustered(cmd, frame, descriptorManager, globalDSetComponent.globalDSets, pipelineManager,
		                            swapChain.swapChainExtent.width, swapChain.swapChainExtent.height,
		                            bufferManager.getBuffer(globalDSetComponent.forwardClusteredGridBuffer, frame),
		                            bufferManager.getBuffer(globalDSetComponent.forwardClusteredInfoBuffer, frame));
	           });
}
