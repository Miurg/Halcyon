#include "PassCommands.hpp"

#include "../Components/DescriptorManagerComponent.hpp"
#include "../Components/DrawInfoComponent.hpp"
#include "../Components/DirectLightComponent.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"
#include "../Resources/Components/ModelDSetComponent.hpp"
#include "../Resources/Managers/BufferManager.hpp"
#include "../Resources/Managers/ModelManager.hpp"
#include "../Resources/Managers/TextureManager.hpp"
#include "../Managers/PipelineManager.hpp"

void drawResetInstancePass(vk::raii::CommandBuffer& cmd, uint32_t frame, DescriptorManagerComponent& dManager,
                           ModelDSetComponent& objectDSetComponent, const DrawInfoComponent& drawInfo,
                           PipelineManager& pManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["reset_instance_count"].pipeline);

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["reset_instance_count"].layout, 0,
	                       dManager.descriptorManager->getSet(objectDSetComponent.modelBufferDSet, frame), nullptr);

	struct PushConsts
	{
		uint32_t drawCommandCount;
	} push;

	push.drawCommandCount = drawInfo.totalDrawCount;

	cmd.pushConstants<PushConsts>(*pManager.pipelines["reset_instance_count"].layout, vk::ShaderStageFlagBits::eCompute,
	                              0, push);
	uint32_t groupCountX = (drawInfo.totalDrawCount + 63) / 64;
	if (groupCountX > 0) cmd.dispatch(groupCountX, 1, 1);

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

void drawCullPass(vk::raii::CommandBuffer& cmd, uint32_t frame, DescriptorManagerComponent& dManager,
                  GlobalDSetComponent& globalDSetComponent, ModelDSetComponent& objectDSetComponent,
                  ModelManager& mManager, BufferManager& bManager, const DrawInfoComponent& drawInfo,
                  PipelineManager& pManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["frustum_culling"].pipeline);

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["frustum_culling"].layout, 0,
	                       dManager.descriptorManager->getSet(globalDSetComponent.globalDSets, frame), nullptr);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["frustum_culling"].layout, 1,
	                       dManager.descriptorManager->getSet(objectDSetComponent.modelBufferDSet, frame), nullptr);

	struct PushConsts
	{
		uint32_t objectCount;
	} push;

	push.objectCount = drawInfo.totalObjectCount;

	cmd.pushConstants<PushConsts>(*pManager.pipelines["frustum_culling"].layout, vk::ShaderStageFlagBits::eCompute, 0,
	                              push);
	uint32_t groupCountX = (drawInfo.totalObjectCount + 63) / 64;
	if (groupCountX > 0) cmd.dispatch(groupCountX, 1, 1);

	vk::MemoryBarrier2 cullBarrier;
	cullBarrier.srcStageMask = vk::PipelineStageFlagBits2::eComputeShader;
	cullBarrier.srcAccessMask = vk::AccessFlagBits2::eShaderWrite;
	cullBarrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer | vk::PipelineStageFlagBits2::eComputeShader;
	cullBarrier.dstAccessMask = vk::AccessFlagBits2::eTransferWrite | vk::AccessFlagBits2::eShaderRead;

	vk::DependencyInfo cullDepInfo;
	cullDepInfo.memoryBarrierCount = 1;
	cullDepInfo.pMemoryBarriers = &cullBarrier;
	cmd.pipelineBarrier2(cullDepInfo);

	cmd.fillBuffer(bManager.buffers[objectDSetComponent.drawCountBuffer.id].buffer[frame], 0, sizeof(uint32_t) * 4, 0);

	vk::MemoryBarrier2 fillBarrier;
	fillBarrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
	fillBarrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
	fillBarrier.dstStageMask = vk::PipelineStageFlagBits2::eComputeShader;
	fillBarrier.dstAccessMask = vk::AccessFlagBits2::eShaderWrite | vk::AccessFlagBits2::eShaderRead;

	vk::DependencyInfo fillDepInfo;
	fillDepInfo.memoryBarrierCount = 1;
	fillDepInfo.pMemoryBarriers = &fillBarrier;
	cmd.pipelineBarrier2(fillDepInfo);

	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["frustum_compaction"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["frustum_compaction"].layout, 0,
	                       dManager.descriptorManager->getSet(objectDSetComponent.modelBufferDSet, frame), nullptr);

	struct CompactionPush
	{
		uint32_t drawCommandCount;
		uint32_t outputOffset;
		uint32_t inputOffset;
		uint32_t countIndex;
	} compactPush;

	uint32_t opaqueSingleCount = drawInfo.opaqueSingleCount;
	uint32_t opaqueDoubleCount = drawInfo.opaqueDoubleCount;
	uint32_t alphaSingleCount = drawInfo.alphaSingleCount;
	uint32_t alphaDoubleCount = drawInfo.alphaDoubleCount;

	uint32_t currentOffset = 0;

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

void drawShadowCullPass(vk::raii::CommandBuffer& cmd, uint32_t frame, DescriptorManagerComponent& dManager,
                        GlobalDSetComponent& globalDSetComponent, ModelDSetComponent& objectDSetComponent,
                        ModelManager& mManager, BufferManager& bManager, const DrawInfoComponent& drawInfo,
                        PipelineManager& pManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["shadow_frustum_culling"].pipeline);

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["shadow_frustum_culling"].layout, 0,
	                       dManager.descriptorManager->getSet(globalDSetComponent.globalDSets, frame), nullptr);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["shadow_frustum_culling"].layout, 1,
	                       dManager.descriptorManager->getSet(objectDSetComponent.modelBufferDSet, frame), nullptr);

	struct PushConsts
	{
		uint32_t objectCount;
	} push;

	push.objectCount = drawInfo.totalObjectCount;

	cmd.pushConstants<PushConsts>(*pManager.pipelines["shadow_frustum_culling"].layout,
	                              vk::ShaderStageFlagBits::eCompute, 0, push);
	uint32_t groupCountX = (drawInfo.totalObjectCount + 63) / 64;
	if (groupCountX > 0) cmd.dispatch(groupCountX, 1, 1);

	vk::MemoryBarrier2 cullBarrier;
	cullBarrier.srcStageMask = vk::PipelineStageFlagBits2::eComputeShader;
	cullBarrier.srcAccessMask = vk::AccessFlagBits2::eShaderWrite;
	cullBarrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer | vk::PipelineStageFlagBits2::eComputeShader;
	cullBarrier.dstAccessMask = vk::AccessFlagBits2::eTransferWrite | vk::AccessFlagBits2::eShaderRead;

	vk::DependencyInfo cullDepInfo;
	cullDepInfo.memoryBarrierCount = 1;
	cullDepInfo.pMemoryBarriers = &cullBarrier;
	cmd.pipelineBarrier2(cullDepInfo);

	cmd.fillBuffer(bManager.buffers[objectDSetComponent.drawCountBuffer.id].buffer[frame], 0, sizeof(uint32_t) * 4, 0);

	vk::MemoryBarrier2 fillBarrier;
	fillBarrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
	fillBarrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
	fillBarrier.dstStageMask = vk::PipelineStageFlagBits2::eComputeShader;
	fillBarrier.dstAccessMask = vk::AccessFlagBits2::eShaderWrite | vk::AccessFlagBits2::eShaderRead;

	vk::DependencyInfo fillDepInfo;
	fillDepInfo.memoryBarrierCount = 1;
	fillDepInfo.pMemoryBarriers = &fillBarrier;
	cmd.pipelineBarrier2(fillDepInfo);

	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pManager.pipelines["frustum_compaction"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pManager.pipelines["frustum_compaction"].layout, 0,
	                       dManager.descriptorManager->getSet(objectDSetComponent.modelBufferDSet, frame), nullptr);

	struct CompactionPush
	{
		uint32_t drawCommandCount;
		uint32_t outputOffset;
		uint32_t inputOffset;
		uint32_t countIndex;
	} compactPush;

	uint32_t opaqueSingleCount = drawInfo.opaqueSingleCount;
	uint32_t opaqueDoubleCount = drawInfo.opaqueDoubleCount;
	uint32_t alphaSingleCount = drawInfo.alphaSingleCount;
	uint32_t alphaDoubleCount = drawInfo.alphaDoubleCount;

	uint32_t currentOffset = 0;

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

void drawShadowPass(vk::raii::CommandBuffer& cmd, uint32_t frame, DirectLightComponent& lightTexture,
                    DescriptorManagerComponent& dManager, GlobalDSetComponent& globalDSetComponent,
                    ModelDSetComponent& objectDSetComponent, TextureManager& tManager, ModelManager& mManager,
                    BufferManager& bManager, const DrawInfoComponent& drawInfo, PipelineManager& pManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["shadow"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["shadow"].layout, 0,
	                       dManager.descriptorManager->getSet(globalDSetComponent.globalDSets, frame), nullptr);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pManager.pipelines["shadow"].layout, 1,
	                       dManager.descriptorManager->getSet(objectDSetComponent.modelBufferDSet, frame), nullptr);
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
		currentCommandOffset += opaqueDoubleCount * commandStride;
	}
	currentCountBufferOffset += sizeof(uint32_t);

	if (alphaSingleCount > 0)
	{
		cmd.setCullMode(vk::CullModeFlagBits::eBack);
		cmd.drawIndexedIndirectCount(bManager.buffers[objectDSetComponent.compactedDrawBuffer.id].buffer[frame],
		                             currentCommandOffset,
		                             bManager.buffers[objectDSetComponent.drawCountBuffer.id].buffer[frame],
		                             currentCountBufferOffset, alphaSingleCount, commandStride);
		currentCommandOffset += alphaSingleCount * commandStride;
	}
	currentCountBufferOffset += sizeof(uint32_t);

	if (alphaDoubleCount > 0)
	{
		cmd.setCullMode(vk::CullModeFlagBits::eNone);
		cmd.drawIndexedIndirectCount(bManager.buffers[objectDSetComponent.compactedDrawBuffer.id].buffer[frame],
		                             currentCommandOffset,
		                             bManager.buffers[objectDSetComponent.drawCountBuffer.id].buffer[frame],
		                             currentCountBufferOffset, alphaDoubleCount, commandStride);
	}
}
