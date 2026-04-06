#include "RenderPasses.hpp"

void drawDepthPrepass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, uint32_t currentFrame,
                      BindlessTextureDSetComponent& bindlessTextureDSetComponent, DescriptorManagerComponent& dManager,
                      GlobalDSetComponent& globalDSetComponent, BufferManager& bManager,
                      ModelDSetComponent& objectDSetComponent, ModelManager& mManager,
                      const DrawInfoComponent& drawInfo, PipelineManager& pManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["depth_prepass"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["depth_prepass"].layout, 0,
	                       dManager.descriptorManager->getSet(globalDSetComponent.globalDSets, currentFrame),
	                       nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *pManager.pipelines["depth_prepass"].layout, 1,
	    dManager.descriptorManager->getSet(objectDSetComponent.modelBufferDSet, currentFrame), nullptr);

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

	// 1. Opaque Single-Sided
	if (opaqueSingleCount > 0)
	{
		cmd.setCullMode(vk::CullModeFlagBits::eBack);
		cmd.drawIndexedIndirectCount(bManager.buffers[objectDSetComponent.compactedDrawBuffer.id].buffer[currentFrame],
		                             currentCommandOffset,
		                             bManager.buffers[objectDSetComponent.drawCountBuffer.id].buffer[currentFrame],
		                             currentCountBufferOffset, opaqueSingleCount, commandStride);
		currentCommandOffset += opaqueSingleCount * commandStride;
	}
	currentCountBufferOffset += sizeof(uint32_t);

	// 2. Opaque Double-Sided
	if (opaqueDoubleCount > 0)
	{
		cmd.setCullMode(vk::CullModeFlagBits::eNone);
		cmd.drawIndexedIndirectCount(bManager.buffers[objectDSetComponent.compactedDrawBuffer.id].buffer[currentFrame],
		                             currentCommandOffset,
		                             bManager.buffers[objectDSetComponent.drawCountBuffer.id].buffer[currentFrame],
		                             currentCountBufferOffset, opaqueDoubleCount, commandStride);
	}
}

void RenderPasses::DepthPrepass(SwapChain& swapChain, uint32_t currentFrame,
                                BindlessTextureDSetComponent& bindlessTextureDSetComponent,
                                DescriptorManagerComponent& dManager, GlobalDSetComponent& globalDSetComponent,
                                BufferManager& bManager, ModelDSetComponent& objectDSetComponent,
                                ModelManager& mManager, const DrawInfoComponent& drawInfo, PipelineManager& pManager,
                                GraphicsSettingsComponent& graphicsSettings, RenderGraph& rg)
{
	vk::ClearValue clearDepth0 = vk::ClearDepthStencilValue(0.0f, 0);
	if (graphicsSettings.msaaSamples & vk::SampleCountFlagBits::e1)
	{
		rg.addPass("DepthPrepass",
		           {.depthAttachment = RGAttachmentConfig{"Depth", vk::AttachmentLoadOp::eClear,
		                                                  vk::AttachmentStoreOp::eStore, clearDepth0}},
		           {}, {{"Depth", RGResourceUsage::DepthAttachmentWrite}},
		           [&, currentFrame](vk::raii::CommandBuffer& cmd)
		           {
			           drawDepthPrepass(cmd, swapChain, currentFrame, bindlessTextureDSetComponent,
			                                                  dManager, globalDSetComponent, bManager, objectDSetComponent,
			                                                  mManager, drawInfo, pManager);
		           });
	}
	else
	{
		rg.addPass("DepthPrepass",
		           {.depthAttachment = RGAttachmentConfig{"DepthMSAA", vk::AttachmentLoadOp::eClear,
		                                                  vk::AttachmentStoreOp::eStore, clearDepth0}},
		           {}, {{"DepthMSAA", RGResourceUsage::DepthAttachmentWrite}},
		           [&, currentFrame](vk::raii::CommandBuffer& cmd)
		           {
			           drawDepthPrepass(cmd, swapChain, currentFrame, bindlessTextureDSetComponent,
			                                                  dManager, globalDSetComponent, bManager, objectDSetComponent,
			                                                  mManager, drawInfo, pManager);
		           });
	}
}