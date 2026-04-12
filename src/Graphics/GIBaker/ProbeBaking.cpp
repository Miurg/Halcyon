#include "LightProbeGIBaking.hpp"

// Inline image barrier for a single array layer (baseArrayLayer support)
static void transitionLayer(vk::raii::CommandBuffer& cmd, vk::Image image, uint32_t baseLayer,
                            vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::AccessFlags2 srcAccess,
                            vk::AccessFlags2 dstAccess, vk::PipelineStageFlags2 srcStage,
                            vk::PipelineStageFlags2 dstStage, vk::ImageAspectFlags aspect)
{
	vk::ImageMemoryBarrier2 barrier;
	barrier.srcStageMask = srcStage;
	barrier.srcAccessMask = srcAccess;
	barrier.dstStageMask = dstStage;
	barrier.dstAccessMask = dstAccess;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
	barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
	barrier.image = image;
	barrier.subresourceRange = {aspect, 0, 1, baseLayer, 1};

	vk::DependencyInfo depInfo;
	depInfo.imageMemoryBarrierCount = 1;
	depInfo.pImageMemoryBarriers = &barrier;
	cmd.pipelineBarrier2(depInfo);
}

static CameraStructure buildFaceCamera(const glm::vec3& probePos, const glm::vec3& forward, const glm::vec3& up)
{
	const glm::mat4 view = glm::lookAtRH(probePos, probePos + forward, up);
	// Reversed-Z perspective
	glm::mat4 proj = glm::perspectiveRH_ZO(glm::radians(90.0f), 1.0f, 1000.0f, 0.01f);
	// Y-flipped for Vulkan
	proj[1][1] *= -1.0f;

	CameraStructure cam{};
	cam.cameraSpaceMatrix = proj * view;
	cam.viewMatrix = view;
	cam.projMatrix = proj;
	cam.invViewProj = glm::inverse(cam.cameraSpaceMatrix);
	cam.cameraPositionAndPadding = glm::vec4(probePos, 0.0f);

	// Same formula as CameraMatrixSystem
	const glm::mat4 transposeSpaceMatrix = glm::transpose(cam.cameraSpaceMatrix);
	cam.frustumPlanes[0] = glm::normalize(transposeSpaceMatrix[3] + transposeSpaceMatrix[0]);
	cam.frustumPlanes[1] = glm::normalize(transposeSpaceMatrix[3] - transposeSpaceMatrix[0]);
	cam.frustumPlanes[2] = glm::normalize(transposeSpaceMatrix[3] + transposeSpaceMatrix[1]);
	cam.frustumPlanes[3] = glm::normalize(transposeSpaceMatrix[3] - transposeSpaceMatrix[1]);
	cam.frustumPlanes[4] = glm::normalize(transposeSpaceMatrix[2]);
	cam.frustumPlanes[5] = glm::normalize(transposeSpaceMatrix[3] - transposeSpaceMatrix[2]);

	return cam;
}

static void drawGeometry(vk::raii::CommandBuffer& cmd, const BakeContext& ctx)
{
	const uint32_t commandStride = sizeof(VkDrawIndexedIndirectCommand);

	// Bind vertex + index buffers (shared across both pipelines)
	cmd.bindVertexBuffers(
	    0, ctx.modelManager->vertexIndexBuffers[ctx.modelManager->meshes[0].vertexIndexBufferID].vertexBuffer, {0});
	cmd.bindIndexBuffer(
	    ctx.modelManager->vertexIndexBuffers[ctx.modelManager->meshes[0].vertexIndexBufferID].indexBuffer, 0,
	    vk::IndexType::eUint32);

	// === Opaque ===
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,
	                 *ctx.pipelineManager->pipelines["global_illumination_forward"].pipeline);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *ctx.pipelineManager->pipelines["global_illumination_forward"].layout, 0,
	    ctx.descriptorManagerComponent->descriptorManager->getSet(ctx.globalDSet->globalDSets, 0), nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *ctx.pipelineManager->pipelines["global_illumination_forward"].layout, 1,
	    ctx.descriptorManagerComponent->descriptorManager->getSet(ctx.modelDSet->modelBufferDSet, 0), nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *ctx.pipelineManager->pipelines["global_illumination_forward"].layout, 2,
	    ctx.descriptorManagerComponent->descriptorManager->getSet(ctx.bindlessDSet->bindlessTextureSet), nullptr);

	uint32_t cmdOffset = 0;
	uint32_t countOffset = 0;

	if (ctx.drawInfo->opaqueSingleCount > 0)
	{
		cmd.setCullMode(vk::CullModeFlagBits::eBack);
		cmd.drawIndexedIndirectCount(ctx.bufferManager->buffers[ctx.modelDSet->compactedDrawBuffer.id].buffer[0],
		                             cmdOffset, ctx.bufferManager->buffers[ctx.modelDSet->drawCountBuffer.id].buffer[0],
		                             countOffset, ctx.drawInfo->opaqueSingleCount, commandStride);
		cmdOffset += ctx.drawInfo->opaqueSingleCount * commandStride;
	}
	countOffset += sizeof(uint32_t);

	if (ctx.drawInfo->opaqueDoubleCount > 0)
	{
		cmd.setCullMode(vk::CullModeFlagBits::eNone);
		cmd.drawIndexedIndirectCount(ctx.bufferManager->buffers[ctx.modelDSet->compactedDrawBuffer.id].buffer[0],
		                             cmdOffset, ctx.bufferManager->buffers[ctx.modelDSet->drawCountBuffer.id].buffer[0],
		                             countOffset, ctx.drawInfo->opaqueDoubleCount, commandStride);
		cmdOffset += ctx.drawInfo->opaqueDoubleCount * commandStride;
	}

	if (ctx.hasSkybox)
	{
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *ctx.pipelineManager->pipelines["skybox_capture"].pipeline);
		cmd.setCullMode(vk::CullModeFlagBits::eNone);
		cmd.draw(3, 1, 0, 0);
	}

	// === Alpha ===
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,
	                 *ctx.pipelineManager->pipelines["global_illumination_forward_alpha"].pipeline);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *ctx.pipelineManager->pipelines["global_illumination_forward_alpha"].layout, 0,
	    ctx.descriptorManagerComponent->descriptorManager->getSet(ctx.globalDSet->globalDSets, 0), nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *ctx.pipelineManager->pipelines["global_illumination_forward_alpha"].layout, 1,
	    ctx.descriptorManagerComponent->descriptorManager->getSet(ctx.modelDSet->modelBufferDSet, 0), nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *ctx.pipelineManager->pipelines["global_illumination_forward_alpha"].layout, 2,
	    ctx.descriptorManagerComponent->descriptorManager->getSet(ctx.bindlessDSet->bindlessTextureSet), nullptr);

	countOffset += sizeof(uint32_t);

	if (ctx.drawInfo->alphaSingleCount > 0)
	{
		cmd.setCullMode(vk::CullModeFlagBits::eBack);
		cmd.drawIndexedIndirectCount(ctx.bufferManager->buffers[ctx.modelDSet->compactedDrawBuffer.id].buffer[0],
		                             cmdOffset, ctx.bufferManager->buffers[ctx.modelDSet->drawCountBuffer.id].buffer[0],
		                             countOffset, ctx.drawInfo->alphaSingleCount, commandStride);
		cmdOffset += ctx.drawInfo->alphaSingleCount * commandStride;
	}
	countOffset += sizeof(uint32_t);

	if (ctx.drawInfo->alphaDoubleCount > 0)
	{
		cmd.setCullMode(vk::CullModeFlagBits::eNone);
		cmd.drawIndexedIndirectCount(ctx.bufferManager->buffers[ctx.modelDSet->compactedDrawBuffer.id].buffer[0],
		                             cmdOffset, ctx.bufferManager->buffers[ctx.modelDSet->drawCountBuffer.id].buffer[0],
		                             countOffset, ctx.drawInfo->alphaDoubleCount, commandStride);
	}
}

static void renderFace(const BakeContext& ctx, const TempImages& tmp, int faceIdx, glm::vec3 probePos,
                       const FaceDesc& face)
{
	// Update camera buffer for this face
	CameraStructure cam = buildFaceCamera(probePos, face.forward, face.up);
	std::memcpy(ctx.bufferManager->buffers[ctx.globalDSet->cameraBuffers.id].bufferMapped[0], &cam,
	            sizeof(CameraStructure));

	// Per-face 2D view into the capture cubemap layer
	vk::raii::ImageView faceView =
	    VulkanUtils::createImageView(tmp.captureImage, kCaptureFormat, vk::ImageAspectFlagBits::eColor, *ctx.device,
	                                 vk::ImageViewType::e2D, 1, 0, 1, static_cast<uint32_t>(faceIdx));

	auto cmd = VulkanUtils::beginSingleTimeCommands(*ctx.device);

	// Frustum cull for this face
	drawResetInstancePass(cmd, 0, *ctx.descriptorManagerComponent, *ctx.modelDSet, *ctx.drawInfo, *ctx.pipelineManager);
	drawCullPass(cmd, 0, *ctx.descriptorManagerComponent, *ctx.globalDSet, *ctx.modelDSet, *ctx.modelManager,
	             *ctx.bufferManager, *ctx.drawInfo, *ctx.pipelineManager);

	// Barrier: compute write -> draw indirect + vertex read
	{
		vk::MemoryBarrier2 barrier;
		barrier.srcStageMask = vk::PipelineStageFlagBits2::eComputeShader;
		barrier.srcAccessMask = vk::AccessFlagBits2::eShaderWrite;
		barrier.dstStageMask = vk::PipelineStageFlagBits2::eDrawIndirect | vk::PipelineStageFlagBits2::eVertexShader;
		barrier.dstAccessMask = vk::AccessFlagBits2::eIndirectCommandRead | vk::AccessFlagBits2::eShaderRead;
		vk::DependencyInfo depInfo;
		depInfo.memoryBarrierCount = 1;
		depInfo.pMemoryBarriers = &barrier;
		cmd.pipelineBarrier2(depInfo);
	}

	// Transition this cubemap face: UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL
	transitionLayer(cmd, tmp.captureImage, static_cast<uint32_t>(faceIdx), vk::ImageLayout::eUndefined,
	                vk::ImageLayout::eColorAttachmentOptimal, vk::AccessFlagBits2::eNone,
	                vk::AccessFlagBits2::eColorAttachmentWrite, vk::PipelineStageFlagBits2::eTopOfPipe,
	                vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::ImageAspectFlagBits::eColor);

	// Attachment setup
	vk::RenderingAttachmentInfo colorAtt;
	colorAtt.imageView = *faceView;
	colorAtt.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	colorAtt.loadOp = vk::AttachmentLoadOp::eClear;
	colorAtt.storeOp = vk::AttachmentStoreOp::eStore;
	colorAtt.clearValue = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);

	vk::RenderingAttachmentInfo depthAtt;
	depthAtt.imageView = *tmp.depthView;
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

	drawGeometry(cmd, ctx);

	cmd.endRendering();

	// Transition face: COLOR_ATTACHMENT_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL for SH projection
	transitionLayer(cmd, tmp.captureImage, static_cast<uint32_t>(faceIdx), vk::ImageLayout::eColorAttachmentOptimal,
	                vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits2::eColorAttachmentWrite,
	                vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eColorAttachmentOutput,
	                vk::PipelineStageFlagBits2::eComputeShader, vk::ImageAspectFlagBits::eColor);

	VulkanUtils::endSingleTimeCommands(cmd, *ctx.device);
}

static void writeProbeMetadata(const BakeContext& ctx, int slot, glm::vec3 pos, float radius)
{
	auto* probes =
	    static_cast<SHProbeEntry*>(ctx.bufferManager->buffers[ctx.globalDSet->shProbeBuffer.id].bufferMapped[0]);

	probes[slot].position = pos;
	probes[slot].influenceRadius = radius;
}

void LightProbeGIBaking::bakeProbe(const BakeContext& ctx, const TempImages& tmp, int slot, glm::vec3 pos, float radius,
                                   vk::ImageView skyboxView, vk::Sampler skyboxSampler)
{
	// Restore the real skybox sampler before capturing faces.
	ctx.descriptorManagerComponent->descriptorManager->updateCubemapSamplerDescriptor(*ctx.bindlessDSet, skyboxView,
	                                                                                  skyboxSampler);

	// Face forward/up vectors - must match faceDirection() in sh_projection.slang
	static constexpr std::array<FaceDesc, 6> faces = {{
	    {{1, 0, 0}, {0, 1, 0}},  // face 0 +X
	    {{-1, 0, 0}, {0, 1, 0}}, // face 1 -X
	    {{0, 1, 0}, {0, 0, -1}}, // face 2 +Y
	    {{0, -1, 0}, {0, 0, 1}}, // face 3 -Y
	    {{0, 0, 1}, {0, 1, 0}},  // face 4 +Z
	    {{0, 0, -1}, {0, 1, 0}}, // face 5 -Z
	}};

	for (int faceIdx = 0; faceIdx < 6; ++faceIdx) renderFace(ctx, tmp, faceIdx, pos, faces[faceIdx]);

	writeProbeMetadata(ctx, slot, pos, radius);

	// Redirect CubemapSampler descriptor to our capture cubemap, then project SH
	ctx.descriptorManagerComponent->descriptorManager->updateCubemapSamplerDescriptor(
	    *ctx.bindlessDSet, ctx.textureManager->textures[tmp.captureHandle.id].textureImageView,
	    ctx.textureManager->textures[tmp.captureHandle.id].textureSampler);

	ctx.bufferManager->bakeSHForProbe(tmp.captureHandle, ctx.globalDSet->shProbeBuffer, slot,
	                                  *ctx.descriptorManagerComponent->descriptorManager, *ctx.bindlessDSet,
	                                  ctx.globalDSet->globalDSets, *ctx.pipelineManager, *ctx.textureManager);
}