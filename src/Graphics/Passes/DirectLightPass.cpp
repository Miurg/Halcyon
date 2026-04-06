#include "RenderPasses.hpp"

void drawShadowCullPass(vk::raii::CommandBuffer& cmd, uint32_t currentFrame,
                                              DescriptorManagerComponent& dManager,
                                              GlobalDSetComponent& globalDSetComponent,
                                              ModelDSetComponent& objectDSetComponent, ModelManager& mManager,
                                              BufferManager& bManager, const DrawInfoComponent& drawInfo,
                                              PipelineManager& pManager)
{
	// === Frustum Culling ===
	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["shadow_frustum_culling"].pipeline);

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["shadow_frustum_culling"].layout, 0,
	                       dManager.descriptorManager->descriptorSets[globalDSetComponent.globalDSets.id][currentFrame],
	                       nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eCompute, *pManager.pipelines["shadow_frustum_culling"].layout, 1,
	    dManager.descriptorManager->descriptorSets[objectDSetComponent.modelBufferDSet.id][currentFrame], nullptr);

	struct PushConsts
	{
		uint32_t objectCount;
	} push;

	push.objectCount = drawInfo.totalObjectCount;

	cmd.pushConstants<PushConsts>(*pManager.pipelines["shadow_frustum_culling"].layout,
	                              vk::ShaderStageFlagBits::eCompute, 0, push);
	uint32_t groupCountX = (drawInfo.totalObjectCount + 63) / 64;
	if (groupCountX > 0) cmd.dispatch(groupCountX, 1, 1);

	// === Barrier: Culling -> fillBuffer ===
	vk::MemoryBarrier2 cullBarrier;
	cullBarrier.srcStageMask = vk::PipelineStageFlagBits2::eComputeShader;
	cullBarrier.srcAccessMask = vk::AccessFlagBits2::eShaderWrite;
	cullBarrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer | vk::PipelineStageFlagBits2::eComputeShader;
	cullBarrier.dstAccessMask = vk::AccessFlagBits2::eTransferWrite | vk::AccessFlagBits2::eShaderRead;

	vk::DependencyInfo cullDepInfo;
	cullDepInfo.memoryBarrierCount = 1;
	cullDepInfo.pMemoryBarriers = &cullBarrier;
	cmd.pipelineBarrier2(cullDepInfo);

	// ===  Zero the draw count buffer ===
	cmd.fillBuffer(bManager.buffers[objectDSetComponent.drawCountBuffer.id].buffer[currentFrame], 0,
	               sizeof(uint32_t) * 4, 0);

	// === Barrier: fillBuffer -> Compaction ===
	vk::MemoryBarrier2 fillBarrier;
	fillBarrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
	fillBarrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
	fillBarrier.dstStageMask = vk::PipelineStageFlagBits2::eComputeShader;
	fillBarrier.dstAccessMask = vk::AccessFlagBits2::eShaderWrite | vk::AccessFlagBits2::eShaderRead;

	vk::DependencyInfo fillDepInfo;
	fillDepInfo.memoryBarrierCount = 1;
	fillDepInfo.pMemoryBarriers = &fillBarrier;
	cmd.pipelineBarrier2(fillDepInfo);

	// === Compaction ===
	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["frustum_compaction"].pipeline);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eCompute, *pManager.pipelines["frustum_compaction"].layout, 0,
	    dManager.descriptorManager->descriptorSets[objectDSetComponent.modelBufferDSet.id][currentFrame], nullptr);

	struct CompactionPush
	{
		uint32_t drawCommandCount;
		uint32_t outputOffset; // Where to start writing in compacted buffer
		uint32_t inputOffset;  // Where to start reading from uncompacted buffer
		uint32_t countIndex;   // 0 for opaque, 1 for alpha test
	} compactPush;

	uint32_t opaqueSingleCount = drawInfo.opaqueSingleCount;
	uint32_t opaqueDoubleCount = drawInfo.opaqueDoubleCount;
	uint32_t alphaSingleCount = drawInfo.alphaSingleCount;
	uint32_t alphaDoubleCount = drawInfo.alphaDoubleCount;

	uint32_t currentOffset = 0;

	// 1. Opaque Single-Sided Compaction
	if (opaqueSingleCount > 0)
	{
		compactPush.drawCommandCount = opaqueSingleCount;
		compactPush.outputOffset = currentOffset;
		compactPush.inputOffset = currentOffset;
		compactPush.countIndex = 0;
		cmd.pushConstants<CompactionPush>(*pManager.pipelines["frustum_compaction"].layout,
		                                  vk::ShaderStageFlagBits::eCompute, 0, compactPush);
		cmd.dispatch((opaqueSingleCount + 63) / 64, 1, 1);
		currentOffset += opaqueSingleCount;
	}

	// 2. Opaque Double-Sided Compaction
	if (opaqueDoubleCount > 0)
	{
		compactPush.drawCommandCount = opaqueDoubleCount;
		compactPush.outputOffset = currentOffset;
		compactPush.inputOffset = currentOffset;
		compactPush.countIndex = 1;
		cmd.pushConstants<CompactionPush>(*pManager.pipelines["frustum_compaction"].layout,
		                                  vk::ShaderStageFlagBits::eCompute, 0, compactPush);
		cmd.dispatch((opaqueDoubleCount + 63) / 64, 1, 1);
		currentOffset += opaqueDoubleCount;
	}

	// 3. Alpha Single-Sided Compaction
	if (alphaSingleCount > 0)
	{
		compactPush.drawCommandCount = alphaSingleCount;
		compactPush.outputOffset = currentOffset;
		compactPush.inputOffset = currentOffset;
		compactPush.countIndex = 2;
		cmd.pushConstants<CompactionPush>(*pManager.pipelines["frustum_compaction"].layout,
		                                  vk::ShaderStageFlagBits::eCompute, 0, compactPush);
		cmd.dispatch((alphaSingleCount + 63) / 64, 1, 1);
		currentOffset += alphaSingleCount;
	}

	// 4. Alpha Double-Sided Compaction
	if (alphaDoubleCount > 0)
	{
		compactPush.drawCommandCount = alphaDoubleCount;
		compactPush.outputOffset = currentOffset;
		compactPush.inputOffset = currentOffset;
		compactPush.countIndex = 3;
		cmd.pushConstants<CompactionPush>(*pManager.pipelines["frustum_compaction"].layout,
		                                  vk::ShaderStageFlagBits::eCompute, 0, compactPush);
		cmd.dispatch((alphaDoubleCount + 63) / 64, 1, 1);
	}

	// === Barrier: Compaction -> DrawIndirect ===
	vk::MemoryBarrier2 drawBarrier;
	drawBarrier.srcStageMask = vk::PipelineStageFlagBits2::eComputeShader;
	drawBarrier.srcAccessMask = vk::AccessFlagBits2::eShaderWrite;
	drawBarrier.dstStageMask = vk::PipelineStageFlagBits2::eDrawIndirect | vk::PipelineStageFlagBits2::eVertexShader;
	drawBarrier.dstAccessMask = vk::AccessFlagBits2::eIndirectCommandRead | vk::AccessFlagBits2::eShaderRead;

	vk::DependencyInfo drawDepInfo;
	drawDepInfo.memoryBarrierCount = 1;
	drawDepInfo.pMemoryBarriers = &drawBarrier;
	cmd.pipelineBarrier2(drawDepInfo);
}

void drawShadowPass(vk::raii::CommandBuffer& cmd, uint32_t currentFrame,
                                          DirectLightComponent& lightTexture, DescriptorManagerComponent& dManager,
                                          GlobalDSetComponent& globalDSetComponent,
                                          ModelDSetComponent& objectDSetComponent, TextureManager& tManager,
                                          ModelManager& mManager, BufferManager& bManager,
                                          const DrawInfoComponent& drawInfo, PipelineManager& pManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["shadow"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["shadow"].layout, 0,
	                       dManager.descriptorManager->descriptorSets[globalDSetComponent.globalDSets.id][currentFrame],
	                       nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *pManager.pipelines["shadow"].layout, 1,
	    dManager.descriptorManager->descriptorSets[objectDSetComponent.modelBufferDSet.id][currentFrame], nullptr);
	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, lightTexture.sizeX, lightTexture.sizeY, 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(lightTexture.sizeX, lightTexture.sizeY)));

	const uint32_t commandStride = sizeof(VkDrawIndexedIndirectCommand);
	cmd.bindVertexBuffers(0, mManager.vertexIndexBuffers[mManager.meshes[0].vertexIndexBufferID].vertexBuffer, {0});
	cmd.bindIndexBuffer(mManager.vertexIndexBuffers[mManager.meshes[0].vertexIndexBufferID].indexBuffer, 0,
	                    vk::IndexType::eUint32);

	uint32_t opaqueSingleCount = drawInfo.opaqueSingleCount;
	uint32_t opaqueDoubleCount = drawInfo.opaqueDoubleCount;
	uint32_t alphaSingleCount = drawInfo.alphaSingleCount;
	uint32_t alphaDoubleCount = drawInfo.alphaDoubleCount;

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
		currentCommandOffset += opaqueDoubleCount * commandStride;
	}
	currentCountBufferOffset += sizeof(uint32_t);

	// 3. Alpha Single-Sided
	if (alphaSingleCount > 0)
	{
		cmd.setCullMode(vk::CullModeFlagBits::eBack);
		cmd.drawIndexedIndirectCount(bManager.buffers[objectDSetComponent.compactedDrawBuffer.id].buffer[currentFrame],
		                             currentCommandOffset,
		                             bManager.buffers[objectDSetComponent.drawCountBuffer.id].buffer[currentFrame],
		                             currentCountBufferOffset, alphaSingleCount, commandStride);
		currentCommandOffset += alphaSingleCount * commandStride;
	}
	currentCountBufferOffset += sizeof(uint32_t);

	// 4. Alpha Double-Sided
	if (alphaDoubleCount > 0)
	{
		cmd.setCullMode(vk::CullModeFlagBits::eNone);
		cmd.drawIndexedIndirectCount(bManager.buffers[objectDSetComponent.compactedDrawBuffer.id].buffer[currentFrame],
		                             currentCommandOffset,
		                             bManager.buffers[objectDSetComponent.drawCountBuffer.id].buffer[currentFrame],
		                             currentCountBufferOffset, alphaDoubleCount, commandStride);
	}
}

void RenderPasses::DirectLightPass(uint32_t currentFrame, DescriptorManagerComponent& dManager,
                                   GlobalDSetComponent& globalDSetComponent, BufferManager& bManager,
                                   ModelDSetComponent& objectDSetComponent, ModelManager& mManager,
                                   const DrawInfoComponent& drawInfo, PipelineManager& pManager, RenderGraph& rg,
                                   TextureManager& textureManager, DirectLightComponent& lightTexture)
{
	vk::ClearValue clearDepth0 = vk::ClearDepthStencilValue(0.0f, 0);
	rg.addPass("ShadowCull", {.isCompute = true}, {}, {},
	           [&, currentFrame](vk::raii::CommandBuffer& cmd)
	           {
		           drawShadowCullPass(cmd, currentFrame, dManager, globalDSetComponent,
		                                                    objectDSetComponent, mManager, bManager, drawInfo, pManager);
	           });

	rg.addPass("Shadow",
	           {.depthAttachment = RGAttachmentConfig{"shadowMap", vk::AttachmentLoadOp::eClear,
	                                                  vk::AttachmentStoreOp::eStore, clearDepth0},
	            .customExtent =
	                vk::Extent2D{static_cast<uint32_t>(lightTexture.sizeX), static_cast<uint32_t>(lightTexture.sizeY)}},
	           {}, {{"shadowMap", RGResourceUsage::DepthAttachmentWrite}},
	           [&, currentFrame](vk::raii::CommandBuffer& cmd)
	           {
		           drawShadowPass(cmd, currentFrame, lightTexture, dManager, globalDSetComponent,
		                                                objectDSetComponent, textureManager, mManager, bManager, drawInfo,
		                                                pManager);
	           });
}