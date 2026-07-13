#include "LightProbeGIBaking.hpp"

struct BakeFacePush
{
	glm::vec3 probePos;
	uint32_t faceIdx;
};

static void drawGeometry(vk::raii::CommandBuffer& cmd, const BakeContext& ctx, glm::vec3 probePos, uint32_t region,
                         int faceIdx)
{
	const uint32_t commandStride = sizeof(VkDrawIndexedIndirectCommand);
	const BakeFacePush facePush{probePos, static_cast<uint32_t>(faceIdx)};

	// Bind vertex + index buffers (shared across both pipelines)
	cmd.bindVertexBuffers(
	    0, ctx.modelManager->getVertexIndexBuffer(0).vertexBuffer, {0});
	cmd.bindIndexBuffer(
	    ctx.modelManager->getVertexIndexBuffer(0).indexBuffer, 0,
	    vk::IndexType::eUint32);

	// === Opaque ===
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,
	                 *ctx.pipelineManager->pipelines["global_illumination_forward"].pipeline);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *ctx.pipelineManager->pipelines["global_illumination_forward"].layout, 0,
	    ctx.descriptorManagerComponent->descriptorManager->getSet(ctx.globalDSet->globalDSets, 0), nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *ctx.pipelineManager->pipelines["global_illumination_forward"].layout, 1,
	    ctx.descriptorManagerComponent->descriptorManager->getSet(ctx.modelDSet->bakeModelDSet), nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *ctx.pipelineManager->pipelines["global_illumination_forward"].layout, 2,
	    ctx.descriptorManagerComponent->descriptorManager->getSet(ctx.bindlessDSet->bindlessTextureSet), nullptr);
	cmd.pushConstants<BakeFacePush>(*ctx.pipelineManager->pipelines["global_illumination_forward"].layout,
	                                vk::ShaderStageFlagBits::eVertex, 0, facePush);

	const uint32_t segmentCounts[6] = {ctx.drawInfo->opaqueSingleCount, ctx.drawInfo->opaqueDoubleCount,
	                                   ctx.drawInfo->maskSingleCount,   ctx.drawInfo->maskDoubleCount,
	                                   ctx.drawInfo->blendSingleCount,  ctx.drawInfo->blendDoubleCount};

	uint32_t cmdOffset = region * ctx.drawInfo->totalDrawCount * commandStride;
	uint32_t countOffset = region * 6u * static_cast<uint32_t>(sizeof(uint32_t));

	auto drawSegment = [&](uint32_t seg)
	{
		uint32_t count = segmentCounts[seg];
		if (count > 0)
		{
			// Backfaces must rasterize during the bake — sh_projection derives probe validity from them
			cmd.setCullMode(vk::CullModeFlagBits::eNone);
			cmd.drawIndexedIndirectCount(
			    ctx.bufferManager->getBuffer(ctx.modelDSet->bakeCompactedDrawBuffer), cmdOffset,
			    ctx.bufferManager->getBuffer(ctx.modelDSet->bakeDrawCountBuffer), countOffset, count,
			    commandStride);
			cmdOffset += count * commandStride;
		}
		countOffset += sizeof(uint32_t);
	};

	drawSegment(0);
	drawSegment(1);

	if (ctx.hasSkybox)
	{
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *ctx.pipelineManager->pipelines["skybox_capture"].pipeline);
		cmd.pushConstants<BakeFacePush>(*ctx.pipelineManager->pipelines["skybox_capture"].layout,
		                                vk::ShaderStageFlagBits::eVertex, 0, facePush);
		cmd.setCullMode(vk::CullModeFlagBits::eNone);
		cmd.draw(3, 1, 0, 0);
	}

	// === Alpha (mask + blend) ===
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,
	                 *ctx.pipelineManager->pipelines["global_illumination_forward_alpha"].pipeline);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *ctx.pipelineManager->pipelines["global_illumination_forward_alpha"].layout, 0,
	    ctx.descriptorManagerComponent->descriptorManager->getSet(ctx.globalDSet->globalDSets, 0), nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *ctx.pipelineManager->pipelines["global_illumination_forward_alpha"].layout, 1,
	    ctx.descriptorManagerComponent->descriptorManager->getSet(ctx.modelDSet->bakeModelDSet), nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *ctx.pipelineManager->pipelines["global_illumination_forward_alpha"].layout, 2,
	    ctx.descriptorManagerComponent->descriptorManager->getSet(ctx.bindlessDSet->bindlessTextureSet), nullptr);
	cmd.pushConstants<BakeFacePush>(*ctx.pipelineManager->pipelines["global_illumination_forward_alpha"].layout,
	                                vk::ShaderStageFlagBits::eVertex, 0, facePush);

	drawSegment(2);
	drawSegment(3);
	drawSegment(4);
	drawSegment(5);
}

static void drawLightSources(vk::raii::CommandBuffer& cmd, const BakeContext& ctx, glm::vec3 probePos, int faceIdx)
{
	const uint32_t lightCount = *ctx.bufferManager->getMapped<uint32_t>(ctx.globalDSet->pointLightCountBuffer);
	if (lightCount == 0) return;

	struct LightSourcePush
	{
		glm::vec3 probePos;
		float scale;
		uint32_t faceIdx;
	};
	const LightSourcePush push{probePos, 0.15f, static_cast<uint32_t>(faceIdx)};

	auto& pip = ctx.pipelineManager->pipelines["gi_light_source_bake"];
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pip.pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pip.layout, 0,
	                       ctx.descriptorManagerComponent->descriptorManager->getSet(ctx.globalDSet->globalDSets, 0),
	                       nullptr);
	cmd.setCullMode(vk::CullModeFlagBits::eBack);
	cmd.pushConstants<LightSourcePush>(*pip.layout, vk::ShaderStageFlagBits::eVertex, 0, push);
	cmd.draw(384u, lightCount, 0u, 0u);
}

static void recordFace(vk::raii::CommandBuffer& cmd, const BakeContext& ctx, const TempImages& tmp, int faceIdx,
                       glm::vec3 probePos, uint32_t region)
{
	vk::RenderingAttachmentInfo colorAtt;
	colorAtt.imageView = *tmp.faceViews[faceIdx];
	colorAtt.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	colorAtt.loadOp = vk::AttachmentLoadOp::eClear;
	colorAtt.storeOp = vk::AttachmentStoreOp::eStore;
	colorAtt.clearValue = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);

	vk::RenderingAttachmentInfo depthAtt;
	depthAtt.imageView = *tmp.depthViews[faceIdx];
	depthAtt.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
	depthAtt.loadOp = vk::AttachmentLoadOp::eClear;
	depthAtt.storeOp = vk::AttachmentStoreOp::eDontCare;
	depthAtt.clearValue = vk::ClearDepthStencilValue(0.0f, 0); // reversed-Z clear

	std::array<vk::RenderingAttachmentInfo, 1> colorAtts = {colorAtt};

	vk::RenderingInfo renderInfo;
	renderInfo.renderArea = vk::Rect2D{{0, 0}, {kCaptureSize, kCaptureSize}};
	renderInfo.layerCount = 1;
	renderInfo.colorAttachmentCount = static_cast<uint32_t>(colorAtts.size());
	renderInfo.pColorAttachments = colorAtts.data();
	renderInfo.pDepthAttachment = &depthAtt;

	cmd.beginRendering(renderInfo);
	cmd.setViewport(
	    0, vk::Viewport(0.0f, 0.0f, static_cast<float>(kCaptureSize), static_cast<float>(kCaptureSize), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D({0, 0}, {kCaptureSize, kCaptureSize}));

	drawGeometry(cmd, ctx, probePos, region, faceIdx);
	drawLightSources(cmd, ctx, probePos, faceIdx);

	cmd.endRendering();
}

static void writeProbeMetadata(const BakeContext& ctx, int slot, glm::vec3 pos, float radius)
{
	auto* probes = ctx.bufferManager->getMapped<SHProbeEntry>(ctx.globalDSet->shProbeBuffer);

	probes[slot].position = pos;
	probes[slot].influenceRadius = radius;
}

void LightProbeGIBaking::recordProbe(vk::raii::CommandBuffer& cmd, const BakeContext& ctx, const TempImages& tmp,
                                     uint32_t regionBase, int slot, glm::vec3 pos, float radius)
{
	// All capture layers to COLOR (the previous probe's SH projection may still sample them),
	// depth WAW between probes.
	{
		std::array<vk::ImageMemoryBarrier2, 2> barriers;

		barriers[0].srcStageMask = vk::PipelineStageFlagBits2::eComputeShader;
		barriers[0].srcAccessMask = vk::AccessFlagBits2::eNone;
		barriers[0].dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
		barriers[0].dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
		barriers[0].oldLayout = vk::ImageLayout::eUndefined;
		barriers[0].newLayout = vk::ImageLayout::eColorAttachmentOptimal;
		barriers[0].srcQueueFamilyIndex = vk::QueueFamilyIgnored;
		barriers[0].dstQueueFamilyIndex = vk::QueueFamilyIgnored;
		barriers[0].image = tmp.captureImage;
		barriers[0].subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 6};

		barriers[1].srcStageMask = vk::PipelineStageFlagBits2::eLateFragmentTests;
		barriers[1].srcAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
		barriers[1].dstStageMask =
		    vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
		barriers[1].dstAccessMask =
		    vk::AccessFlagBits2::eDepthStencilAttachmentWrite | vk::AccessFlagBits2::eDepthStencilAttachmentRead;
		barriers[1].oldLayout = vk::ImageLayout::eDepthAttachmentOptimal;
		barriers[1].newLayout = vk::ImageLayout::eDepthAttachmentOptimal;
		barriers[1].srcQueueFamilyIndex = vk::QueueFamilyIgnored;
		barriers[1].dstQueueFamilyIndex = vk::QueueFamilyIgnored;
		barriers[1].image = vk::Image(tmp.depthRaw);
		barriers[1].subresourceRange = {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 6};

		vk::DependencyInfo depInfo;
		depInfo.imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size());
		depInfo.pImageMemoryBarriers = barriers.data();
		cmd.pipelineBarrier2(depInfo);
	}

	for (int faceIdx = 0; faceIdx < 6; ++faceIdx)
		recordFace(cmd, ctx, tmp, faceIdx, pos, regionBase + static_cast<uint32_t>(faceIdx));

	// All capture layers -> SHADER_READ for SH projection
	{
		vk::ImageMemoryBarrier2 barrier;
		barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
		barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
		barrier.dstStageMask = vk::PipelineStageFlagBits2::eComputeShader;
		barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
		barrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
		barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
		barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
		barrier.image = tmp.captureImage;
		barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 6};

		vk::DependencyInfo depInfo;
		depInfo.imageMemoryBarrierCount = 1;
		depInfo.pImageMemoryBarriers = &barrier;
		cmd.pipelineBarrier2(depInfo);
	}

	writeProbeMetadata(ctx, slot, pos, radius);

	ctx.bufferManager->recordSHProjection(cmd, static_cast<int>(kCaptureSize), slot,
	                                      *ctx.descriptorManagerComponent->descriptorManager, *ctx.bindlessDSet,
	                                      ctx.globalDSet->globalDSets, *ctx.pipelineManager);
}
