#include "LightProbeGIBaking.hpp"

// Renders the shadow map once for the entire probe grid.
void LightProbeGIBaking::bakeShadowMap(const BakeContext& ctx)
{
	// 1. Read current sun direction from the GPU side sun buffer
	const auto& sunBuffer = ctx.bufferManager->buffers[ctx.globalDSet->sunCameraBuffers.id];
	const DirectLightStructure* existingDirectLight =
	    static_cast<const DirectLightStructure*>(sunBuffer.bufferMapped[0]);
	const glm::vec3 lightDirection = glm::normalize(-glm::vec3(existingDirectLight->direction));

	// 2. Compute probe grid bounding sphere
	const glm::vec3 gridEnd =
	    ctx.grid->origin +
	    ctx.grid->spacing * glm::vec3(ctx.grid->count.x - 1, ctx.grid->count.y - 1, ctx.grid->count.z - 1);
	const glm::vec3 gridCenter = (ctx.grid->origin + gridEnd) * 0.5f;
	// Half diagonal of the grid AABB + one extra spacing as margin
	float gridRadius = glm::length(gridEnd - ctx.grid->origin) * 0.5f + ctx.grid->spacing;

	// 3. Build orthographic light space matrix covering the full grid.
	const float zOffset = gridRadius + ctx.lightComponent->shadowCasterRange;
	const glm::vec3 lightPos = gridCenter - lightDirection * zOffset;
	const glm::mat4 lightView = glm::lookAt(lightPos, gridCenter, glm::vec3(0.0f, 1.0f, 0.0f));
	const float zFarRange = zOffset + gridRadius;

	glm::mat4 lightProj = glm::orthoRH_ZO(-gridRadius, gridRadius, -gridRadius, gridRadius, zFarRange, 0.0f);
	lightProj[1][1] *= -1.0f; // Y-flip for Vulkan

	// Texel-snapping, same as in CameraMatrixSystem
	const float shadowMapWidth = ctx.lightComponent->sizeX;
	glm::mat4 lightSpaceMatrix = lightProj * lightView;
	glm::vec4 shadowOrigin = lightSpaceMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	shadowOrigin *= (shadowMapWidth / 2.0f);
	const glm::vec4 roundOffset = (glm::round(shadowOrigin) - shadowOrigin) * (2.0f / shadowMapWidth);
	lightProj[3][0] += roundOffset.x;
	lightProj[3][1] += roundOffset.y;
	lightSpaceMatrix = lightProj * lightView;

	// 4. Build light frustum planes
	glm::vec4 lightFrustumPlanes[6];
	{
		const glm::mat4 transposeMatrix = glm::transpose(lightSpaceMatrix);
		lightFrustumPlanes[0] = glm::normalize(transposeMatrix[3] + transposeMatrix[0]);
		lightFrustumPlanes[1] = glm::normalize(transposeMatrix[3] - transposeMatrix[0]);
		lightFrustumPlanes[2] = glm::normalize(transposeMatrix[3] + transposeMatrix[1]);
		lightFrustumPlanes[3] = glm::normalize(transposeMatrix[3] - transposeMatrix[1]);
		lightFrustumPlanes[4] = glm::normalize(transposeMatrix[2]);
		lightFrustumPlanes[5] = glm::normalize(transposeMatrix[3] - transposeMatrix[2]);
	}

	// 5. Upload new sun buffer (keep color/ambient/direction)
	DirectLightStructure directLightToReplace = *existingDirectLight;
	directLightToReplace.lightSpaceMatrix = lightSpaceMatrix;
	for (int i = 0; i < 6; ++i) directLightToReplace.frustumPlanes[i] = lightFrustumPlanes[i];
	// Cover the full shadow map in NDC (no camera-frustum tightening)
	directLightToReplace.cameraFrustumLightSpaceBounds = glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);
	std::memcpy(sunBuffer.bufferMapped[0], &directLightToReplace, sizeof(DirectLightStructure));

	// 6. Upload an all-accepting camera frustum.
	CameraStructure infiniteCam{};
	for (int i = 0; i < 6; ++i) infiniteCam.frustumPlanes[i] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	const auto& camBuf = ctx.bufferManager->buffers[ctx.globalDSet->cameraBuffers.id];
	std::memcpy(camBuf.bufferMapped[0], &infiniteCam, sizeof(CameraStructure));

	// 7. GPU work: reset -> shadow cull -> shadow render.
	auto cmd = VulkanUtils::beginSingleTimeCommands(*ctx.device);

	drawResetInstancePass(cmd, 0, *ctx.descriptorManagerComponent, *ctx.modelDSet, *ctx.drawInfo, *ctx.pipelineManager);
	drawShadowCullPass(cmd, 0, *ctx.descriptorManagerComponent, *ctx.globalDSet, *ctx.modelDSet, *ctx.modelManager,
	                   *ctx.bufferManager, *ctx.drawInfo, *ctx.pipelineManager);

	// Transition shadow map: UNDEFINED -> DEPTH_ATTACHMENT_OPTIMAL
	vk::Image shadowImage = ctx.textureManager->textures[ctx.lightComponent->textureShadowImage.id].textureImage;
	VulkanUtils::transitionImageLayout(
	    cmd, shadowImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal,
	    vk::AccessFlagBits2::eNone, vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
	    vk::PipelineStageFlagBits2::eTopOfPipe, vk::PipelineStageFlagBits2::eEarlyFragmentTests,
	    vk::ImageAspectFlagBits::eDepth, 1, 1);

	vk::RenderingAttachmentInfo depthAtt;
	depthAtt.imageView = ctx.textureManager->textures[ctx.lightComponent->textureShadowImage.id].textureImageView;
	depthAtt.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
	depthAtt.loadOp = vk::AttachmentLoadOp::eClear;
	depthAtt.storeOp = vk::AttachmentStoreOp::eStore;
	depthAtt.clearValue = vk::ClearDepthStencilValue(0.0f, 0); // reversed-Z

	vk::RenderingInfo renderInfo;
	renderInfo.renderArea = vk::Rect2D{
	    {0, 0}, {static_cast<uint32_t>(ctx.lightComponent->sizeX), static_cast<uint32_t>(ctx.lightComponent->sizeY)}};
	renderInfo.layerCount = 1;
	renderInfo.pDepthAttachment = &depthAtt;

	cmd.beginRendering(renderInfo);
	drawShadowPass(cmd, 0, *ctx.lightComponent, *ctx.descriptorManagerComponent, *ctx.globalDSet, *ctx.modelDSet,
	               *ctx.textureManager, *ctx.modelManager, *ctx.bufferManager, *ctx.drawInfo, *ctx.pipelineManager);
	cmd.endRendering();

	// Transition shadow map: DEPTH_ATTACHMENT_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
	VulkanUtils::transitionImageLayout(
	    cmd, shadowImage, vk::ImageLayout::eDepthAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
	    vk::AccessFlagBits2::eDepthStencilAttachmentWrite, vk::AccessFlagBits2::eShaderRead,
	    vk::PipelineStageFlagBits2::eLateFragmentTests, vk::PipelineStageFlagBits2::eFragmentShader,
	    vk::ImageAspectFlagBits::eDepth, 1, 1);

	VulkanUtils::endSingleTimeCommands(cmd, *ctx.device);
}
