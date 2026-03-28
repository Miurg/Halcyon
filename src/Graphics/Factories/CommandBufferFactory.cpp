#include "CommandBufferFactory.hpp"
#include <imgui.h>
#include <imgui_impl_vulkan.h>

void CommandBufferFactory::drawShadowCullPass(vk::raii::CommandBuffer& cmd,
                                              uint32_t currentFrame, DescriptorManagerComponent& dManager,
                                              GlobalDSetComponent* globalDSetComponent,
                                              ModelDSetComponent* objectDSetComponent, ModelManager& mManager,
                                              BufferManager& bManager, const DrawInfoComponent& drawInfo, PipelineManager& pManager)
{
	// === Frustum Culling ===
	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["shadow_frustum_culling"].pipeline);

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["shadow_frustum_culling"].layout, 0,
	                       dManager.descriptorManager->descriptorSets[globalDSetComponent->globalDSets.id][currentFrame],
	                       nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eCompute, *pManager.pipelines["shadow_frustum_culling"].layout, 1,
	    dManager.descriptorManager->descriptorSets[objectDSetComponent->modelBufferDSet.id][currentFrame], nullptr);

	struct PushConsts
	{
		uint32_t objectCount;
	} push;

	push.objectCount = drawInfo.totalObjectCount;

	cmd.pushConstants<PushConsts>(*pManager.pipelines["shadow_frustum_culling"].layout,
	                              vk::ShaderStageFlagBits::eCompute, 0,
	                              push);
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
	cmd.fillBuffer(bManager.buffers[objectDSetComponent->drawCountBuffer.id].buffer[currentFrame], 0,
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
	    dManager.descriptorManager->descriptorSets[objectDSetComponent->modelBufferDSet.id][currentFrame], nullptr);

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

void CommandBufferFactory::drawShadowPass(vk::raii::CommandBuffer& cmd,
                                          uint32_t currentFrame, DirectLightComponent& lightTexture,
                                          DescriptorManagerComponent& dManager,
                                          GlobalDSetComponent* globalDSetComponent,
                                          ModelDSetComponent* objectDSetComponent, TextureManager& tManager,
                                          ModelManager& mManager, BufferManager& bManager,
                                          const DrawInfoComponent& drawInfo, PipelineManager& pManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["shadow"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["shadow"].layout, 0,
	                       dManager.descriptorManager->descriptorSets[globalDSetComponent->globalDSets.id][currentFrame],
	                       nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *pManager.pipelines["shadow"].layout, 1,
	    dManager.descriptorManager->descriptorSets[objectDSetComponent->modelBufferDSet.id][currentFrame], nullptr);
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
		cmd.drawIndexedIndirectCount(bManager.buffers[objectDSetComponent->compactedDrawBuffer.id].buffer[currentFrame],
		                             currentCommandOffset,
		                             bManager.buffers[objectDSetComponent->drawCountBuffer.id].buffer[currentFrame],
		                             currentCountBufferOffset, opaqueSingleCount, commandStride);
		currentCommandOffset += opaqueSingleCount * commandStride;
	}
	currentCountBufferOffset += sizeof(uint32_t);

	// 2. Opaque Double-Sided
	if (opaqueDoubleCount > 0)
	{
		cmd.setCullMode(vk::CullModeFlagBits::eNone);
		cmd.drawIndexedIndirectCount(bManager.buffers[objectDSetComponent->compactedDrawBuffer.id].buffer[currentFrame],
		                             currentCommandOffset,
		                             bManager.buffers[objectDSetComponent->drawCountBuffer.id].buffer[currentFrame],
		                             currentCountBufferOffset, opaqueDoubleCount, commandStride);
		currentCommandOffset += opaqueDoubleCount * commandStride;
	}
	currentCountBufferOffset += sizeof(uint32_t);

	// 3. Alpha Single-Sided
	if (alphaSingleCount > 0)
	{
		cmd.setCullMode(vk::CullModeFlagBits::eBack);
		cmd.drawIndexedIndirectCount(bManager.buffers[objectDSetComponent->compactedDrawBuffer.id].buffer[currentFrame],
		                             currentCommandOffset,
		                             bManager.buffers[objectDSetComponent->drawCountBuffer.id].buffer[currentFrame],
		                             currentCountBufferOffset, alphaSingleCount, commandStride);
		currentCommandOffset += alphaSingleCount * commandStride;
	}
	currentCountBufferOffset += sizeof(uint32_t);

	// 4. Alpha Double-Sided
	if (alphaDoubleCount > 0)
	{
		cmd.setCullMode(vk::CullModeFlagBits::eNone);
		cmd.drawIndexedIndirectCount(bManager.buffers[objectDSetComponent->compactedDrawBuffer.id].buffer[currentFrame],
		                             currentCommandOffset,
		                             bManager.buffers[objectDSetComponent->drawCountBuffer.id].buffer[currentFrame],
		                             currentCountBufferOffset, alphaDoubleCount, commandStride);
	}
}

void CommandBufferFactory::drawResetInstancePass(vk::raii::CommandBuffer& cmd,
                                                 uint32_t currentFrame, DescriptorManagerComponent& dManager,
                                                 ModelDSetComponent* objectDSetComponent,
                                                 const DrawInfoComponent& drawInfo, PipelineManager& pManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["reset_instance_count"].pipeline);

	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eCompute, *pManager.pipelines["reset_instance_count"].layout, 0,
	    dManager.descriptorManager->descriptorSets[objectDSetComponent->modelBufferDSet.id][currentFrame], nullptr);

	struct PushConsts
	{
		uint32_t drawCommandCount;
	} push;

	push.drawCommandCount = drawInfo.totalDrawCount;

	cmd.pushConstants<PushConsts>(*pManager.pipelines["reset_instance_count"].layout, vk::ShaderStageFlagBits::eCompute, 0,
	                              push);
	uint32_t groupCountX = (drawInfo.totalDrawCount + 63) / 64;
	if (groupCountX > 0) cmd.dispatch(groupCountX, 1, 1);

	// === Barrier: Reset -> Cull ===
	vk::MemoryBarrier2 resetBarrier;
	resetBarrier.srcStageMask = vk::PipelineStageFlagBits2::eComputeShader;
	resetBarrier.srcAccessMask = vk::AccessFlagBits2::eShaderWrite;
	resetBarrier.dstStageMask = vk::PipelineStageFlagBits2::eComputeShader;
	resetBarrier.dstAccessMask = vk::AccessFlagBits2::eShaderWrite | vk::AccessFlagBits2::eShaderRead;

	vk::DependencyInfo resetDepInfo;
	resetDepInfo.memoryBarrierCount = 1;
	resetDepInfo.pMemoryBarriers = &resetBarrier;
	cmd.pipelineBarrier2(resetDepInfo);
}

void CommandBufferFactory::drawCullPass(vk::raii::CommandBuffer& cmd,
                                        uint32_t currentFrame, DescriptorManagerComponent& dManager,
                                        GlobalDSetComponent* globalDSetComponent,
                                        ModelDSetComponent* objectDSetComponent, ModelManager& mManager,
                                        BufferManager& bManager, const DrawInfoComponent& drawInfo, PipelineManager& pManager)
{
	// === Frustum Culling ===
	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["frustum_culling"].pipeline);

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["frustum_culling"].layout, 0,
	                       dManager.descriptorManager->descriptorSets[globalDSetComponent->globalDSets.id][currentFrame],
	                       nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eCompute, *pManager.pipelines["frustum_culling"].layout, 1,
	    dManager.descriptorManager->descriptorSets[objectDSetComponent->modelBufferDSet.id][currentFrame], nullptr);

	struct PushConsts
	{
		uint32_t objectCount;
	} push;

	push.objectCount = drawInfo.totalObjectCount;

	cmd.pushConstants<PushConsts>(*pManager.pipelines["frustum_culling"].layout, vk::ShaderStageFlagBits::eCompute, 0,
	                              push);
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
	cmd.fillBuffer(bManager.buffers[objectDSetComponent->drawCountBuffer.id].buffer[currentFrame], 0,
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
	    dManager.descriptorManager->descriptorSets[objectDSetComponent->modelBufferDSet.id][currentFrame], nullptr);

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

void CommandBufferFactory::drawDepthPrepass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, uint32_t currentFrame,
                                            BindlessTextureDSetComponent& bindlessTextureDSetComponent,
                                            DescriptorManagerComponent& dManager,
                                            GlobalDSetComponent* globalDSetComponent, BufferManager& bManager,
                                            ModelDSetComponent* objectDSetComponent, ModelManager& mManager,
                                            const DrawInfoComponent& drawInfo, PipelineManager& pManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["depth_prepass"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["depth_prepass"].layout, 0,
	                       dManager.descriptorManager->descriptorSets[globalDSetComponent->globalDSets.id][currentFrame],
	                       nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *pManager.pipelines["depth_prepass"].layout, 1,
	    dManager.descriptorManager->descriptorSets[objectDSetComponent->modelBufferDSet.id][currentFrame], nullptr);

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
		cmd.drawIndexedIndirectCount(bManager.buffers[objectDSetComponent->compactedDrawBuffer.id].buffer[currentFrame],
		                             currentCommandOffset,
		                             bManager.buffers[objectDSetComponent->drawCountBuffer.id].buffer[currentFrame],
		                             currentCountBufferOffset, opaqueSingleCount, commandStride);
		currentCommandOffset += opaqueSingleCount * commandStride;
	}
	currentCountBufferOffset += sizeof(uint32_t);

	// 2. Opaque Double-Sided
	if (opaqueDoubleCount > 0)
	{
		cmd.setCullMode(vk::CullModeFlagBits::eNone);
		cmd.drawIndexedIndirectCount(bManager.buffers[objectDSetComponent->compactedDrawBuffer.id].buffer[currentFrame],
		                             currentCommandOffset,
		                             bManager.buffers[objectDSetComponent->drawCountBuffer.id].buffer[currentFrame],
		                             currentCountBufferOffset, opaqueDoubleCount, commandStride);
	}
}

void CommandBufferFactory::drawMainPass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, uint32_t currentFrame,
                                        BindlessTextureDSetComponent& bindlessTextureDSetComponent,
                                        DescriptorManagerComponent& dManager, GlobalDSetComponent* globalDSetComponent,
                                        BufferManager& bManager, ModelDSetComponent* objectDSetComponent,
                                        ModelManager& mManager, const DrawInfoComponent& drawInfo, PipelineManager& pManager)
{
	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
	                                static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["standard_forward_opaque"].layout, 0,
	                       dManager.descriptorManager->descriptorSets[globalDSetComponent->globalDSets.id][currentFrame],
	                       nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *pManager.pipelines["standard_forward_opaque"].layout, 1,
	    dManager.descriptorManager->descriptorSets[objectDSetComponent->modelBufferDSet.id][currentFrame], nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *pManager.pipelines["standard_forward_opaque"].layout, 2,
	    dManager.descriptorManager->descriptorSets[bindlessTextureDSetComponent.bindlessTextureSet.id][0], nullptr);

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

	// Opaque passes
	if (opaqueSingleCount > 0 || opaqueDoubleCount > 0)
	{
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["standard_forward_opaque"].pipeline);

		// 1. Opaque Single-Sided
		if (opaqueSingleCount > 0)
		{
			cmd.setCullMode(vk::CullModeFlagBits::eBack);
			cmd.drawIndexedIndirectCount(
			    bManager.buffers[objectDSetComponent->compactedDrawBuffer.id].buffer[currentFrame], currentCommandOffset,
			    bManager.buffers[objectDSetComponent->drawCountBuffer.id].buffer[currentFrame], currentCountBufferOffset,
			    opaqueSingleCount, commandStride);
			currentCommandOffset += opaqueSingleCount * commandStride;
		}
		currentCountBufferOffset += sizeof(uint32_t);

		// 2. Opaque Double-Sided
		if (opaqueDoubleCount > 0)
		{
			cmd.setCullMode(vk::CullModeFlagBits::eNone);
			cmd.drawIndexedIndirectCount(
			    bManager.buffers[objectDSetComponent->compactedDrawBuffer.id].buffer[currentFrame], currentCommandOffset,
			    bManager.buffers[objectDSetComponent->drawCountBuffer.id].buffer[currentFrame], currentCountBufferOffset,
			    opaqueDoubleCount, commandStride);
			currentCommandOffset += opaqueDoubleCount * commandStride;
		}
		currentCountBufferOffset += sizeof(uint32_t);
	}
	else
	{
		// Advance offsets if opaque passes are entirely skipped but we need to reach alpha passes
		currentCommandOffset += (opaqueSingleCount + opaqueDoubleCount) * commandStride;
		currentCountBufferOffset += 2 * sizeof(uint32_t);
	}
	
	// Skybox pass
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["skybox"].pipeline);
	cmd.setCullMode(vk::CullModeFlagBits::eNone);
	cmd.draw(3, 1, 0, 0);

	// Alpha test passes
	if (alphaSingleCount > 0 || alphaDoubleCount > 0)
	{
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["standard_forward_alpha"].pipeline);

		// 3. Alpha Single-Sided
		if (alphaSingleCount > 0)
		{
			cmd.setCullMode(vk::CullModeFlagBits::eBack);
			cmd.drawIndexedIndirectCount(
			    bManager.buffers[objectDSetComponent->compactedDrawBuffer.id].buffer[currentFrame], currentCommandOffset,
			    bManager.buffers[objectDSetComponent->drawCountBuffer.id].buffer[currentFrame], currentCountBufferOffset,
			    alphaSingleCount, commandStride);
			currentCommandOffset += alphaSingleCount * commandStride;
		}
		currentCountBufferOffset += sizeof(uint32_t);

		// 4. Alpha Double-Sided
		if (alphaDoubleCount > 0)
		{
			cmd.setCullMode(vk::CullModeFlagBits::eNone);
			cmd.drawIndexedIndirectCount(
			    bManager.buffers[objectDSetComponent->compactedDrawBuffer.id].buffer[currentFrame], currentCommandOffset,
			    bManager.buffers[objectDSetComponent->drawCountBuffer.id].buffer[currentFrame], currentCountBufferOffset,
			    alphaDoubleCount, commandStride);
		}
	}
}

void CommandBufferFactory::drawFxaaPass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, DescriptorManagerComponent& dManager,
                                        DSetHandle fxaaDescriptorSetIndex, PipelineManager& pManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["fxaa"].pipeline);

	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
	                                static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["fxaa"].layout, 0,
	                       dManager.descriptorManager->descriptorSets[fxaaDescriptorSetIndex.id][0], nullptr);

	struct PushConstants
	{
		float rcpFrame[2];
	} push;
	push.rcpFrame[0] = 1.0f / static_cast<float>(swapChain.swapChainExtent.width);
	push.rcpFrame[1] = 1.0f / static_cast<float>(swapChain.swapChainExtent.height);

	cmd.pushConstants<PushConstants>(*pManager.pipelines["fxaa"].layout, vk::ShaderStageFlagBits::eFragment, 0, push);
	cmd.setCullMode(vk::CullModeFlagBits::eNone);
	cmd.draw(3, 1, 0, 0);
}

void CommandBufferFactory::drawSsaoPass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, DescriptorManagerComponent& dManager,
                                        DSetHandle ssaoDescriptorSetIndex, DSetHandle globalDescriptorSetIndex,
                                        const SsaoSettingsComponent& ssaoSettings, PipelineManager& pManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pManager.pipelines["ssao"].pipeline);

	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width / 2),
	                                static_cast<float>(swapChain.swapChainExtent.height / 2), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D{swapChain.swapChainExtent.width / 2,
	                                                              swapChain.swapChainExtent.height / 2}));

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pManager.pipelines["ssao"].layout, 0,
	                       dManager.descriptorManager->descriptorSets[ssaoDescriptorSetIndex.id][0], nullptr);

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pManager.pipelines["ssao"].layout, 1,
	                       dManager.descriptorManager->descriptorSets[globalDescriptorSetIndex.id][0], nullptr);

	struct SsaoPushConstants
	{
		int kernelSize;
		float radius;
		float bias;
		float power;
		int numDirections;
		float maxScreenRadius;
		float fadeStart;
		float fadeEnd;
		float texelSize[2];
	} push;
	push.kernelSize = ssaoSettings.kernelSize;
	push.radius = ssaoSettings.radius;
	push.bias = ssaoSettings.bias;
	push.power = ssaoSettings.power;
	push.numDirections = ssaoSettings.numDirections;
	push.maxScreenRadius = ssaoSettings.maxScreenRadius;
	push.fadeStart = ssaoSettings.fadeStart;
	push.fadeEnd = ssaoSettings.fadeEnd;
	push.texelSize[0] = 1.0f / static_cast<float>(swapChain.swapChainExtent.width / 2);
	push.texelSize[1] = 1.0f / static_cast<float>(swapChain.swapChainExtent.height / 2);

	cmd.pushConstants<SsaoPushConstants>(pManager.pipelines["ssao"].layout, vk::ShaderStageFlagBits::eFragment, 0,
	                                     push);
	cmd.setCullMode(vk::CullModeFlagBits::eNone);
	cmd.draw(3, 1, 0, 0);
}

void CommandBufferFactory::drawSsaoBlurPass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, DescriptorManagerComponent& dManager, DSetHandle ssaoBlurDescriptorSetIndex,
                                            PipelineManager& pManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["ssao_blur"].pipeline);

	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width / 2),
	                                static_cast<float>(swapChain.swapChainExtent.height / 2), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D{swapChain.swapChainExtent.width / 2,
	                                                              swapChain.swapChainExtent.height / 2}));

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["ssao_blur"].layout, 0,
	                       dManager.descriptorManager->descriptorSets[ssaoBlurDescriptorSetIndex.id][0], nullptr);

	struct BlurPushConstants
	{
		float texelSize[2];
	} push;
	push.texelSize[0] = 1.0f / static_cast<float>(swapChain.swapChainExtent.width / 2);
	push.texelSize[1] = 1.0f / static_cast<float>(swapChain.swapChainExtent.height / 2);

	cmd.pushConstants<BlurPushConstants>(*pManager.pipelines["ssao_blur"].layout, vk::ShaderStageFlagBits::eFragment, 0,
	                                     push);
	cmd.setCullMode(vk::CullModeFlagBits::eNone);
	cmd.draw(3, 1, 0, 0);
}

void CommandBufferFactory::drawSSAOApplyPass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, DescriptorManagerComponent& dManager,
                                             DSetHandle ssaoApplyDescriptorSetIndex, PipelineManager& pManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["ssao_apply"].pipeline);

	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
											  static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["ssao_apply"].layout, 0,
	                       dManager.descriptorManager->descriptorSets[ssaoApplyDescriptorSetIndex.id][0], nullptr);

	cmd.setCullMode(vk::CullModeFlagBits::eNone);
	cmd.draw(3, 1, 0, 0);
}

void CommandBufferFactory::drawToneMappingPass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, DescriptorManagerComponent& dManager,
                                               DSetHandle toneMappingDescriptorSetIndex, PipelineManager& pManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["tone_mapping"].pipeline);

	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
	                                static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["tone_mapping"].layout, 0,
	                       dManager.descriptorManager->descriptorSets[toneMappingDescriptorSetIndex.id][0], nullptr);

	cmd.setCullMode(vk::CullModeFlagBits::eNone);
	cmd.draw(3, 1, 0, 0);
}

void CommandBufferFactory::drawBloomDownsamplePass(vk::raii::CommandBuffer& cmd, 
                                                    DescriptorManagerComponent& dManager,
                                                   DSetHandle dSetHandle, PipelineManager& pManager,
                                                    float texelSizeX, float texelSizeY, float threshold, float knee,
                                                    int isFirstPass, vk::Extent2D extent)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["bloom_downsample"].pipeline);

	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(extent.width),
	                                static_cast<float>(extent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), extent));

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["bloom_downsample"].layout, 0,
	                       dManager.descriptorManager->descriptorSets[dSetHandle.id][0], nullptr);

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

	cmd.pushConstants<PushConstants>(*pManager.pipelines["bloom_downsample"].layout,
	                                vk::ShaderStageFlagBits::eFragment, 0, push);
	cmd.setCullMode(vk::CullModeFlagBits::eNone);
	cmd.draw(3, 1, 0, 0);
}

void CommandBufferFactory::drawBloomUpsamplePass(vk::raii::CommandBuffer& cmd, 
                                                  DescriptorManagerComponent& dManager,
                                                 DSetHandle dSetHandle, PipelineManager& pManager,
                                                  float texelSizeX, float texelSizeY, float blendFactor,
                                                  int isLastPass, vk::Extent2D extent)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["bloom_upsample"].pipeline);

	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(extent.width),
	                                static_cast<float>(extent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), extent));

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["bloom_upsample"].layout, 0,
	                       dManager.descriptorManager->descriptorSets[dSetHandle.id][0], nullptr);

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

	cmd.pushConstants<PushConstants>(*pManager.pipelines["bloom_upsample"].layout,
	                                vk::ShaderStageFlagBits::eFragment, 0, push);
	cmd.setCullMode(vk::CullModeFlagBits::eNone);
	cmd.draw(3, 1, 0, 0);
}

void CommandBufferFactory::drawImGui(vk::raii::CommandBuffer& cmd)
{
	// ImGui
	ImDrawData* draw_data = ImGui::GetDrawData();
	if (draw_data)
	{
		ImGui_ImplVulkan_RenderDrawData(draw_data, *cmd);
	}
}


void CommandBufferFactory::transitionImageLayout(vk::raii::CommandBuffer& commandBuffer, vk::Image image,
                                                 vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                                                 vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask,
                                                 vk::PipelineStageFlags2 srcStageMask,
                                                 vk::PipelineStageFlags2 dstStageMask,
                                                 vk::ImageAspectFlags imageAspectFlags, uint32_t layerCount,
                                                 uint32_t mipLevelCount)
{
	vk::ImageMemoryBarrier2 barrier;
	barrier.srcStageMask = srcStageMask;
	barrier.srcAccessMask = srcAccessMask;
	barrier.dstStageMask = dstStageMask;
	barrier.dstAccessMask = dstAccessMask;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	barrier.subresourceRange.aspectMask = imageAspectFlags;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevelCount;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = layerCount;

	vk::DependencyInfo dependencyInfo;
	dependencyInfo.dependencyFlags = {};
	dependencyInfo.imageMemoryBarrierCount = 1;
	dependencyInfo.pImageMemoryBarriers = &barrier;
	commandBuffer.pipelineBarrier2(dependencyInfo);
}