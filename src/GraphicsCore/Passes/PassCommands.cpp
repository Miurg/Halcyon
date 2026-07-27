#include "GraphicsCore/Passes/PassCommands.hpp"
#include "GraphicsCore/Passes/DrawVariant.hpp"

#include "GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include "GraphicsCore/Components/DrawInfoComponent.hpp"
#include "GraphicsCore/Components/DirectLightComponent.hpp"
#include "GraphicsCore/Resources/Components/GlobalDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/ModelDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include "GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "GraphicsCore/Resources/Managers/ModelManager.hpp"
#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "GraphicsCore/Managers/PipelineManager.hpp"
#include <array>

void recordSHProjection(vk::raii::CommandBuffer& cmd, int cubemapResolution, int probeSlot,
                        DescriptorManager& descriptorManager, BindlessTextureDSetComponent& dSetComponent,
                        DSetHandle globalDSet, PipelineManager& pipelineManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pipelineManager.pipelines["sh_projection"].pipeline);

	// sh_projection: set 0 = globalSet (SHProbeEntry[] output), set 1 = textureSet (cubemap input)
	std::array<vk::DescriptorSet, 2> sets = {
	    descriptorManager.getSet(globalDSet),
	    descriptorManager.getSet(dSetComponent.bindlessTextureSet),
	};
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pipelineManager.pipelines["sh_projection"].layout, 0, sets,
	                       nullptr);

	struct PushData
	{
		int cubemapResolution;
		int probeSlot;
	};
	PushData pushData = {cubemapResolution, probeSlot};
	cmd.pushConstants(*pipelineManager.pipelines["sh_projection"].layout, vk::ShaderStageFlagBits::eCompute, 0,
	                  vk::ArrayProxy<const PushData>(1, &pushData));

	cmd.dispatch(1, 1, 1);

	vk::MemoryBarrier2 barrier;
	barrier.srcStageMask = vk::PipelineStageFlagBits2::eComputeShader;
	barrier.srcAccessMask = vk::AccessFlagBits2::eShaderWrite;
	barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
	barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;

	vk::DependencyInfo depInfo;
	depInfo.memoryBarrierCount = 1;
	depInfo.pMemoryBarriers = &barrier;
	cmd.pipelineBarrier2(depInfo);
}

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

	cmd.fillBuffer(bufferManager.getBuffer(objectDSetComponent.drawCountBuffer, frame), 0,
	               sizeof(uint32_t) * drawInfo.segments.size(), 0);

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

	uint32_t currentOffset = 0;
	for (uint32_t i = 0; i < drawInfo.segments.size(); ++i)
	{
		uint32_t count = drawInfo.segments[i].maxCount;
		if (count == 0) continue;
		compactPush.drawCommandCount = count;
		compactPush.outputOffset = currentOffset;
		compactPush.inputOffset = currentOffset;
		compactPush.countIndex = i;
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

	cmd.fillBuffer(bufferManager.getBuffer(objectDSetComponent.drawCountBuffer, frame), 0,
	               sizeof(uint32_t) * drawInfo.segments.size(), 0);

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

	uint32_t inputOffset = 0;
	uint32_t outputOffset = 0;
	uint32_t countIdx = 0;
	for (auto& seg : drawInfo.segments)
	{
		if (kDrawVariants[seg.variantIndex].isTransparent) { inputOffset += seg.maxCount; continue; }
		if (seg.maxCount > 0)
		{
			compactPush.drawCommandCount = seg.maxCount;
			compactPush.outputOffset = outputOffset;
			compactPush.inputOffset = inputOffset;
			compactPush.countIndex = countIdx;
			cmd.pushConstants<CompactionPush>(*pipelineManager.pipelines["frustum_compaction"].layout,
			                                  vk::ShaderStageFlagBits::eCompute, 0, compactPush);
			cmd.dispatch((seg.maxCount + 63) / 64, 1, 1);
		}
		inputOffset += seg.maxCount;
		outputOffset += seg.maxCount;
		countIdx++;
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
	auto& firstLayout = pipelineManager.pipelines["standard_opaque_shadow"].layout;
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *firstLayout, 0,
	                       descriptorManager.descriptorManager->getSet(globalDSetComponent.globalDSets, frame), nullptr);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *firstLayout, 1,
	                       descriptorManager.descriptorManager->getSet(objectDSetComponent.modelBufferDSet, frame), nullptr);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *firstLayout, 2,
	                       descriptorManager.descriptorManager->getSet(bTextureDSet.bindlessTextureSet), nullptr);
	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, lightTexture.sizeX, lightTexture.sizeY, 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(lightTexture.sizeX, lightTexture.sizeY)));

	cmd.bindVertexBuffers(0, modelManager.getVertexIndexBuffer(0).vertexBuffer, {0});
	cmd.bindIndexBuffer(modelManager.getVertexIndexBuffer(0).indexBuffer, 0,
	                    vk::IndexType::eUint32);

	DrawCursor cursor{bufferManager.getBuffer(objectDSetComponent.compactedDrawBuffer, frame),
	                  bufferManager.getBuffer(objectDSetComponent.drawCountBuffer, frame)};

	std::string_view prevPipeline;
	for (auto& seg : drawInfo.segments)
	{
		auto& var = kDrawVariants[seg.variantIndex];
		if (var.isTransparent) continue;
		std::string key = std::string(var.pipeline) + "_shadow";
		if (var.pipeline != prevPipeline)
		{
			cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineManager.pipelines[key].pipeline);
			prevPipeline = var.pipeline;
		}
		cursor.draw(cmd, seg.maxCount, var.cullMode);
	}
}
