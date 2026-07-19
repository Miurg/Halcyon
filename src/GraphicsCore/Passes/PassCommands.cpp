#include "GraphicsCore/Passes/PassCommands.hpp"

#include "GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include "GraphicsCore/Components/DrawInfoComponent.hpp"
#include "GraphicsCore/Components/DirectLightComponent.hpp"
#include "GraphicsCore/Resources/Components/GlobalDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/ModelDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include "GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "GraphicsCore/Resources/Managers/ModelManager.hpp"
#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Managers/PipelineManager.hpp"

void drawResetInstancePass(vk::raii::CommandBuffer& cmd, uint32_t frame, DescriptorManagerComponent& descriptorManager,
                           ModelDSetComponent& objectDSetComponent, const DrawInfoComponent& drawInfo,
                           PipelineManager& pipelineManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pipelineManager.pipelines["reset_instance_count"].pipeline);

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pipelineManager.pipelines["reset_instance_count"].layout, 0,
	                       descriptorManager.descriptorManager->getSet(objectDSetComponent.modelBufferDSet, frame), nullptr);

	struct PushConsts
	{
		uint32_t drawCommandCount;
	} push;

	push.drawCommandCount = drawInfo.totalDrawCount;

	cmd.pushConstants<PushConsts>(*pipelineManager.pipelines["reset_instance_count"].layout, vk::ShaderStageFlagBits::eCompute,
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

void drawCullPass(vk::raii::CommandBuffer& cmd, uint32_t frame, DescriptorManagerComponent& descriptorManager,
                  GlobalDSetComponent& globalDSetComponent, ModelDSetComponent& objectDSetComponent,
                  ModelManager& modelManager, BufferManager& bufferManager, const DrawInfoComponent& drawInfo,
                  PipelineManager& pipelineManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pipelineManager.pipelines["frustum_culling"].pipeline);

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pipelineManager.pipelines["frustum_culling"].layout, 0,
	                       descriptorManager.descriptorManager->getSet(globalDSetComponent.globalDSets, frame), nullptr);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pipelineManager.pipelines["frustum_culling"].layout, 1,
	                       descriptorManager.descriptorManager->getSet(objectDSetComponent.modelBufferDSet, frame), nullptr);

	struct PushConsts
	{
		uint32_t objectCount;
	} push;

	push.objectCount = drawInfo.totalObjectCount;

	cmd.pushConstants<PushConsts>(*pipelineManager.pipelines["frustum_culling"].layout, vk::ShaderStageFlagBits::eCompute, 0,
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

	cmd.fillBuffer(bufferManager.getBuffer(objectDSetComponent.drawCountBuffer, frame), 0, sizeof(uint32_t) * 6, 0);

	vk::MemoryBarrier2 fillBarrier;
	fillBarrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
	fillBarrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
	fillBarrier.dstStageMask = vk::PipelineStageFlagBits2::eComputeShader;
	fillBarrier.dstAccessMask = vk::AccessFlagBits2::eShaderWrite | vk::AccessFlagBits2::eShaderRead;

	vk::DependencyInfo fillDepInfo;
	fillDepInfo.memoryBarrierCount = 1;
	fillDepInfo.pMemoryBarriers = &fillBarrier;
	cmd.pipelineBarrier2(fillDepInfo);

	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pipelineManager.pipelines["frustum_compaction"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pipelineManager.pipelines["frustum_compaction"].layout, 0,
	                       descriptorManager.descriptorManager->getSet(objectDSetComponent.modelBufferDSet, frame), nullptr);

	struct CompactionPush
	{
		uint32_t drawCommandCount;
		uint32_t outputOffset;
		uint32_t inputOffset;
		uint32_t countIndex;
	} compactPush;

	const uint32_t segmentCounts[6] = {drawInfo.opaqueSingleCount, drawInfo.opaqueDoubleCount,
	                                   drawInfo.maskSingleCount,   drawInfo.maskDoubleCount,
	                                   drawInfo.blendSingleCount,  drawInfo.blendDoubleCount};

	uint32_t currentOffset = 0;
	for (uint32_t segment = 0; segment < 6; ++segment)
	{
		uint32_t count = segmentCounts[segment];
		if (count == 0) continue;
		compactPush.drawCommandCount = count;
		compactPush.outputOffset = currentOffset;
		compactPush.inputOffset = currentOffset;
		compactPush.countIndex = segment;
		cmd.pushConstants<CompactionPush>(*pipelineManager.pipelines["frustum_compaction"].layout,
		                                  vk::ShaderStageFlagBits::eCompute, 0, compactPush);
		cmd.dispatch((count + 63) / 64, 1, 1);
		currentOffset += count;
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

void drawShadowCullPass(vk::raii::CommandBuffer& cmd, uint32_t frame, DescriptorManagerComponent& descriptorManager,
                        GlobalDSetComponent& globalDSetComponent, ModelDSetComponent& objectDSetComponent,
                        ModelManager& modelManager, BufferManager& bufferManager, const DrawInfoComponent& drawInfo,
                        PipelineManager& pipelineManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pipelineManager.pipelines["shadow_frustum_culling"].pipeline);

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pipelineManager.pipelines["shadow_frustum_culling"].layout, 0,
	                       descriptorManager.descriptorManager->getSet(globalDSetComponent.globalDSets, frame), nullptr);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pipelineManager.pipelines["shadow_frustum_culling"].layout, 1,
	                       descriptorManager.descriptorManager->getSet(objectDSetComponent.modelBufferDSet, frame), nullptr);

	struct PushConsts
	{
		uint32_t objectCount;
	} push;

	push.objectCount = drawInfo.totalObjectCount;

	cmd.pushConstants<PushConsts>(*pipelineManager.pipelines["shadow_frustum_culling"].layout,
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

	cmd.fillBuffer(bufferManager.getBuffer(objectDSetComponent.drawCountBuffer, frame), 0, sizeof(uint32_t) * 6, 0);

	vk::MemoryBarrier2 fillBarrier;
	fillBarrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
	fillBarrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
	fillBarrier.dstStageMask = vk::PipelineStageFlagBits2::eComputeShader;
	fillBarrier.dstAccessMask = vk::AccessFlagBits2::eShaderWrite | vk::AccessFlagBits2::eShaderRead;

	vk::DependencyInfo fillDepInfo;
	fillDepInfo.memoryBarrierCount = 1;
	fillDepInfo.pMemoryBarriers = &fillBarrier;
	cmd.pipelineBarrier2(fillDepInfo);

	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pipelineManager.pipelines["frustum_compaction"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pipelineManager.pipelines["frustum_compaction"].layout, 0,
	                       descriptorManager.descriptorManager->getSet(objectDSetComponent.modelBufferDSet, frame), nullptr);

	struct CompactionPush
	{
		uint32_t drawCommandCount;
		uint32_t outputOffset;
		uint32_t inputOffset;
		uint32_t countIndex;
	} compactPush;

	// BLEND (segs 4,5) is excluded from shadow casting.
	const uint32_t segmentCounts[4] = {drawInfo.opaqueSingleCount, drawInfo.opaqueDoubleCount,
	                                   drawInfo.maskSingleCount,    drawInfo.maskDoubleCount};

	uint32_t currentOffset = 0;
	for (uint32_t segment = 0; segment < 4; ++segment)
	{
		uint32_t count = segmentCounts[segment];
		if (count == 0) continue;
		compactPush.drawCommandCount = count;
		compactPush.outputOffset = currentOffset;
		compactPush.inputOffset = currentOffset;
		compactPush.countIndex = segment;
		cmd.pushConstants<CompactionPush>(*pipelineManager.pipelines["frustum_compaction"].layout,
		                                  vk::ShaderStageFlagBits::eCompute, 0, compactPush);
		cmd.dispatch((count + 63) / 64, 1, 1);
		currentOffset += count;
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
                    DescriptorManagerComponent& descriptorManager, GlobalDSetComponent& globalDSetComponent,
                    ModelDSetComponent& objectDSetComponent, BindlessTextureDSetComponent& bTextureDSet,
                    TextureManager& textureManager, ModelManager& modelManager, BufferManager& bufferManager,
                    const DrawInfoComponent& drawInfo, PipelineManager& pipelineManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineManager.pipelines["shadow"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineManager.pipelines["shadow"].layout, 0,
	                       descriptorManager.descriptorManager->getSet(globalDSetComponent.globalDSets, frame), nullptr);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineManager.pipelines["shadow"].layout, 1,
	                       descriptorManager.descriptorManager->getSet(objectDSetComponent.modelBufferDSet, frame), nullptr);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineManager.pipelines["shadow"].layout, 2,
	                       descriptorManager.descriptorManager->getSet(bTextureDSet.bindlessTextureSet), nullptr);
	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, lightTexture.sizeX, lightTexture.sizeY, 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(lightTexture.sizeX, lightTexture.sizeY)));

	const uint32_t commandStride = sizeof(VkDrawIndexedIndirectCommand);
	cmd.bindVertexBuffers(0, modelManager.getVertexIndexBuffer(0).vertexBuffer, {0});
	cmd.bindIndexBuffer(modelManager.getVertexIndexBuffer(0).indexBuffer, 0,
	                    vk::IndexType::eUint32);

	// BLEND (segs 4,5) is excluded from shadow casting.
	const uint32_t segmentCounts[4] = {drawInfo.opaqueSingleCount, drawInfo.opaqueDoubleCount,
	                                   drawInfo.maskSingleCount,    drawInfo.maskDoubleCount};

	uint32_t currentCommandOffset = 0;
	uint32_t currentCountBufferOffset = 0;

	for (uint32_t segment = 0; segment < 4; ++segment)
	{
		// MASK (segs 2,3) casts through the fragment shadow pipeline.
		if (segment == 2) cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineManager.pipelines["shadow_alpha"].pipeline);

		uint32_t count = segmentCounts[segment];
		if (count > 0)
		{
			cmd.setCullMode((segment & 1) ? vk::CullModeFlagBits::eNone : vk::CullModeFlagBits::eBack);
			cmd.drawIndexedIndirectCount(bufferManager.getBuffer(objectDSetComponent.compactedDrawBuffer, frame),
			                             currentCommandOffset,
			                             bufferManager.getBuffer(objectDSetComponent.drawCountBuffer, frame),
			                             currentCountBufferOffset, count, commandStride);
			currentCommandOffset += count * commandStride;
		}
		currentCountBufferOffset += sizeof(uint32_t);
	}
}
