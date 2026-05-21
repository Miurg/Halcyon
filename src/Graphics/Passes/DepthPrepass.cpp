#include "DepthPrepass.hpp"

#include <Orhescyon/GeneralManager.hpp>

#include "../GraphicsContexts.hpp"
#include "../SwapChain.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../Components/BufferManagerComponent.hpp"
#include "../Components/ModelManagerComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Components/PipelineManagerComponent.hpp"
#include "../Components/GraphicsSettingsComponent.hpp"
#include "../Components/DrawInfoComponent.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"
#include "../Resources/Components/ModelDSetComponent.hpp"
#include "../Resources/Managers/BufferManager.hpp"
#include "../Resources/Managers/ModelManager.hpp"
#include "../Managers/PipelineManager.hpp"
#include "../RenderGraph/RenderGraph.hpp"

void DepthPrepass::draw(vk::raii::CommandBuffer& cmd, uint32_t frame, SwapChain& swapChain,
                        DescriptorManagerComponent& dManager, GlobalDSetComponent& globalDSetComponent,
                        BufferManager& bManager, ModelDSetComponent& objectDSetComponent, ModelManager& mManager,
                        const DrawInfoComponent& drawInfo, PipelineManager& pManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["depth_prepass"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["depth_prepass"].layout, 0,
	                       dManager.descriptorManager->getSet(globalDSetComponent.globalDSets, frame), nullptr);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["depth_prepass"].layout, 1,
	                       dManager.descriptorManager->getSet(objectDSetComponent.modelBufferDSet, frame), nullptr);

	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
	                                static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

	const uint32_t commandStride = sizeof(VkDrawIndexedIndirectCommand);
	cmd.bindVertexBuffers(0, mManager.vertexIndexBuffers[mManager.meshes[0].vertexIndexBufferID].vertexBuffer, {0});
	cmd.bindIndexBuffer(mManager.vertexIndexBuffers[mManager.meshes[0].vertexIndexBufferID].indexBuffer, 0,
	                    vk::IndexType::eUint32);
	uint32_t opaqueSingleCount = drawInfo.opaqueSingleCount;
	uint32_t opaqueDoubleCount = drawInfo.opaqueDoubleCount;

	uint32_t currentCommandOffset = 0;
	uint32_t currentCountBufferOffset = 0;

	if (opaqueSingleCount > 0)
	{
		cmd.setCullMode(vk::CullModeFlagBits::eBack);
		cmd.drawIndexedIndirectCount(bManager.buffers[objectDSetComponent.compactedDrawBuffer.id].buffer[frame],
		                             currentCommandOffset,
		                             bManager.buffers[objectDSetComponent.drawCountBuffer.id].buffer[frame],
		                             currentCountBufferOffset, opaqueSingleCount, commandStride);
		currentCommandOffset += opaqueSingleCount * commandStride;
	}
	currentCountBufferOffset += sizeof(uint32_t);

	if (opaqueDoubleCount > 0)
	{
		cmd.setCullMode(vk::CullModeFlagBits::eNone);
		cmd.drawIndexedIndirectCount(bManager.buffers[objectDSetComponent.compactedDrawBuffer.id].buffer[frame],
		                             currentCommandOffset,
		                             bManager.buffers[objectDSetComponent.drawCountBuffer.id].buffer[frame],
		                             currentCountBufferOffset, opaqueDoubleCount, commandStride);
	}
}

void DepthPrepass::addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame)
{
	auto& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& globalDSetComponent = *gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	auto& bManager = *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	auto& objectDSetComponent = *gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();
	auto& mManager = *gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;
	auto& drawInfo = *gm.getContextComponent<CurrentFrameContext, DrawInfoComponent>();
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	auto& graphicsSettings = *gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>();

	vk::ClearValue clearDepth0 = vk::ClearDepthStencilValue(0.0f, 0);
	const char* depthName = (graphicsSettings.msaaSamples & vk::SampleCountFlagBits::e1) ? "Depth" : "DepthMSAA";

	rg.addPass("DepthPrepass",
	           {.depthAttachment = RGAttachmentConfig{depthName, vk::AttachmentLoadOp::eClear,
	                                                  vk::AttachmentStoreOp::eStore, clearDepth0}},
	           {}, {{depthName, RGResourceUsage::DepthAttachmentWrite}},
	           [&, frame](vk::raii::CommandBuffer& cmd)
	           {
		           draw(cmd, frame, swapChain, dManager, globalDSetComponent, bManager, objectDSetComponent, mManager,
		                drawInfo, pManager);
	           });
}
