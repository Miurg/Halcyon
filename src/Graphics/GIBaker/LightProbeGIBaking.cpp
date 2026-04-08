#include "LightProbeGIBaking.hpp"

#include "../GraphicsContexts.hpp"
#include "../Components/LightProbeGridComponent.hpp"
#include "../Components/VulkanDeviceComponent.hpp"
#include "../Components/BufferManagerComponent.hpp"
#include "../Components/TextureManagerComponent.hpp"
#include "../Components/ModelManagerComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Components/PipelineManagerComponent.hpp"
#include "../Components/DrawInfoComponent.hpp"
#include "../Components/SkyboxComponent.hpp"
#include "../Components/VMAllocatorComponent.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"
#include "../Resources/Components/ModelDSetComponent.hpp"
#include "../Resources/Components/BindlessTextureDSetComponent.hpp"
#include "../Resources/ResourceStructures.hpp"
#include "../Resources/Managers/BufferManager.hpp"
#include "../Resources/Managers/TextureManager.hpp"
#include "../Resources/Managers/DescriptorManager.hpp"
#include "../Managers/PipelineManager.hpp"
#include "../Passes/RenderPasses.hpp"
#include "../VulkanUtils.hpp"
#include "../VulkanConst.hpp"

#include <vk_mem_alloc.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <array>
#include <iostream>

// ─────────────────────────────────────────────────────────────────────────────
// File-scope constants
// ─────────────────────────────────────────────────────────────────────────────

static constexpr uint32_t kCaptureSize = 128;
static constexpr vk::Format kCaptureFormat = vk::Format::eR16G16B16A16Sfloat;

// ─────────────────────────────────────────────────────────────────────────────
// Implementation-only structs
// ─────────────────────────────────────────────────────────────────────────────

struct FaceDesc
{
	glm::vec3 forward, up;
};

struct BakeContext
{
	VulkanDevice* device;
	BufferManager* bufferManager;
	TextureManager* textureManager;
	ModelManager* modelManager;
	DescriptorManagerComponent* descriptorManager;
	PipelineManager* pipelineManager;
	GlobalDSetComponent* globalDSet;
	ModelDSetComponent* modelDSet;
	BindlessTextureDSetComponent* bindlessDSet;
	SkyboxComponent* skybox;
	DrawInfoComponent* drawInfo;
	LightProbeGridComponent* grid;
	VmaAllocator allocator;
};

struct TempImages
{
	TextureHandle captureHandle;
	vk::Image captureImage{}; // non-owning 
	VkImage depthRaw{};
	VmaAllocation depthAlloc{};
	vk::raii::ImageView depthView{nullptr};
	VkImage dummyRaw{};
	VmaAllocation dummyAlloc{};
	vk::raii::ImageView dummyView{nullptr};
};

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
	const glm::mat4 t = glm::transpose(cam.cameraSpaceMatrix);
	cam.frustumPlanes[0] = glm::normalize(t[3] + t[0]);
	cam.frustumPlanes[1] = glm::normalize(t[3] - t[0]);
	cam.frustumPlanes[2] = glm::normalize(t[3] + t[1]);
	cam.frustumPlanes[3] = glm::normalize(t[3] - t[1]);
	cam.frustumPlanes[4] = glm::normalize(t[2]);
	cam.frustumPlanes[5] = glm::normalize(t[3] - t[2]);

	return cam;
}

static BakeContext gatherContext(GeneralManager& gm)
{
	BakeContext ctx;
	ctx.device = gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	ctx.bufferManager = gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	ctx.textureManager = gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	ctx.modelManager = gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;
	ctx.descriptorManager = gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	ctx.pipelineManager = gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	ctx.globalDSet = gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	ctx.modelDSet = gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();
	ctx.bindlessDSet = gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
	ctx.skybox = gm.getContextComponent<SkyBoxContext, SkyboxComponent>();
	ctx.drawInfo = gm.getContextComponent<CurrentFrameContext, DrawInfoComponent>();
	ctx.grid = gm.getContextComponent<LightProbeGridContext, LightProbeGridComponent>();
	ctx.allocator = gm.getContextComponent<VMAllocatorContext, VMAllocatorComponent>()->allocator;
	return ctx;
}

static TempImages createTempImages(const BakeContext& ctx)
{
	TempImages tmp;

	// Temp capture cubemap
	tmp.captureHandle = ctx.textureManager->createCubemapImage(kCaptureSize, kCaptureSize, kCaptureFormat,
	                                                           vk::ImageUsageFlagBits::eColorAttachment |
	                                                               vk::ImageUsageFlagBits::eSampled);

	Texture& captureTex = ctx.textureManager->textures[tmp.captureHandle.id];
	tmp.captureImage = captureTex.textureImage;
	ctx.textureManager->createCubemapImageView(captureTex, kCaptureFormat, vk::ImageAspectFlagBits::eColor);
	ctx.textureManager->createCubemapSampler(captureTex);

	// Depth image (VMA-owned)
	const vk::Format depthFormat = ctx.textureManager->findBestFormat();

	VkImageCreateInfo depthCI{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	depthCI.imageType = VK_IMAGE_TYPE_2D;
	depthCI.format = static_cast<VkFormat>(depthFormat);
	depthCI.extent = {kCaptureSize, kCaptureSize, 1};
	depthCI.mipLevels = 1;
	depthCI.arrayLayers = 1;
	depthCI.samples = VK_SAMPLE_COUNT_1_BIT;
	depthCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	depthCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	depthCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VmaAllocationCreateInfo depthAllocCI{};
	depthAllocCI.usage = VMA_MEMORY_USAGE_AUTO;

	if (vmaCreateImage(ctx.allocator, &depthCI, &depthAllocCI, &tmp.depthRaw, &tmp.depthAlloc, nullptr) != VK_SUCCESS)
		throw std::runtime_error("[LightProbeGISystem] Failed to create temp depth image.");

	vk::Image depthImageWrap(tmp.depthRaw);
	tmp.depthView =
	    VulkanUtils::createImageView(depthImageWrap, depthFormat, vk::ImageAspectFlagBits::eDepth, *ctx.device);

	{
		auto cmd = VulkanUtils::beginSingleTimeCommands(*ctx.device);
		VulkanUtils::transitionImageLayout(
		    cmd, depthImageWrap, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal,
		    vk::AccessFlagBits2::eNone, vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
		    vk::PipelineStageFlagBits2::eTopOfPipe, vk::PipelineStageFlagBits2::eEarlyFragmentTests,
		    vk::ImageAspectFlagBits::eDepth, 1, 1);
		VulkanUtils::endSingleTimeCommands(cmd, *ctx.device);
	}

	// Dummy vsNormal attachment (capture pipeline writes Location 1 - needs a valid target)
	VkImageCreateInfo dummyCI{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	dummyCI.imageType = VK_IMAGE_TYPE_2D;
	dummyCI.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	dummyCI.extent = {kCaptureSize, kCaptureSize, 1};
	dummyCI.mipLevels = 1;
	dummyCI.arrayLayers = 1;
	dummyCI.samples = VK_SAMPLE_COUNT_1_BIT;
	dummyCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	dummyCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	dummyCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VmaAllocationCreateInfo dummyAllocCI{};
	dummyAllocCI.usage = VMA_MEMORY_USAGE_AUTO;

	if (vmaCreateImage(ctx.allocator, &dummyCI, &dummyAllocCI, &tmp.dummyRaw, &tmp.dummyAlloc, nullptr) != VK_SUCCESS)
		throw std::runtime_error("[LightProbeGISystem] Failed to create dummy vsNormal image.");

	vk::Image dummyImageWrap(tmp.dummyRaw);
	tmp.dummyView = VulkanUtils::createImageView(dummyImageWrap, vk::Format::eR16G16B16A16Sfloat,
	                                             vk::ImageAspectFlagBits::eColor, *ctx.device);

	{
		auto cmd = VulkanUtils::beginSingleTimeCommands(*ctx.device);
		VulkanUtils::transitionImageLayout(
		    cmd, dummyImageWrap, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
		    vk::AccessFlagBits2::eNone, vk::AccessFlagBits2::eColorAttachmentWrite,
		    vk::PipelineStageFlagBits2::eTopOfPipe, vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		    vk::ImageAspectFlagBits::eColor, 1, 1);
		VulkanUtils::endSingleTimeCommands(cmd, *ctx.device);
	}

	return tmp;
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
	                 *ctx.pipelineManager->pipelines["standard_forward_capture"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
	                       *ctx.pipelineManager->pipelines["standard_forward_capture"].layout, 0,
	                       ctx.descriptorManager->descriptorManager->getSet(ctx.globalDSet->globalDSets, 0), nullptr);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
	                       *ctx.pipelineManager->pipelines["standard_forward_capture"].layout, 1,
	                       ctx.descriptorManager->descriptorManager->getSet(ctx.modelDSet->modelBufferDSet, 0), nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *ctx.pipelineManager->pipelines["standard_forward_capture"].layout, 2,
	    ctx.descriptorManager->descriptorManager->getSet(ctx.bindlessDSet->bindlessTextureSet), nullptr);

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

	// === Alpha ===
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,
	                 *ctx.pipelineManager->pipelines["standard_forward_capture_alpha"].pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
	                       *ctx.pipelineManager->pipelines["standard_forward_capture_alpha"].layout, 0,
	                       ctx.descriptorManager->descriptorManager->getSet(ctx.globalDSet->globalDSets, 0), nullptr);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
	                       *ctx.pipelineManager->pipelines["standard_forward_capture_alpha"].layout, 1,
	                       ctx.descriptorManager->descriptorManager->getSet(ctx.modelDSet->modelBufferDSet, 0), nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *ctx.pipelineManager->pipelines["standard_forward_capture_alpha"].layout, 2,
	    ctx.descriptorManager->descriptorManager->getSet(ctx.bindlessDSet->bindlessTextureSet), nullptr);

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
	drawResetInstancePass(cmd, 0, *ctx.descriptorManager, *ctx.modelDSet, *ctx.drawInfo, *ctx.pipelineManager);
	drawCullPass(cmd, 0, *ctx.descriptorManager, *ctx.globalDSet, *ctx.modelDSet, *ctx.modelManager, *ctx.bufferManager,
	             *ctx.drawInfo, *ctx.pipelineManager);

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

	vk::RenderingAttachmentInfo dummyAtt;
	dummyAtt.imageView = *tmp.dummyView;
	dummyAtt.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	dummyAtt.loadOp = vk::AttachmentLoadOp::eDontCare;
	dummyAtt.storeOp = vk::AttachmentStoreOp::eDontCare;

	std::array<vk::RenderingAttachmentInfo, 2> colorAtts = {colorAtt, dummyAtt};

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
	// Write probe position + influenceRadius via GPU command for guaranteed GPU visibility.
	struct ProbeMetadata
	{
		glm::vec3 position;
		float influenceRadius;
	};
	const ProbeMetadata meta{pos, radius};
	auto cmd = VulkanUtils::beginSingleTimeCommands(*ctx.device);
	cmd.updateBuffer(ctx.bufferManager->buffers[ctx.globalDSet->shProbeBuffer.id].buffer[0],
	                 static_cast<vk::DeviceSize>(slot) * sizeof(SHProbeEntry),
	                 vk::ArrayProxy<const ProbeMetadata>(1, &meta));
	VulkanUtils::endSingleTimeCommands(cmd, *ctx.device);
}

static void bakeProbe(const BakeContext& ctx, const TempImages& tmp, int slot, glm::vec3 pos, float radius)
{
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
	ctx.descriptorManager->descriptorManager->updateCubemapSamplerDescriptor(
	    *ctx.bindlessDSet, ctx.textureManager->textures[tmp.captureHandle.id].textureImageView,
	    ctx.textureManager->textures[tmp.captureHandle.id].textureSampler);

	ctx.bufferManager->bakeSHForProbe(tmp.captureHandle, ctx.globalDSet->shProbeBuffer, slot,
	                                  *ctx.descriptorManager->descriptorManager, *ctx.bindlessDSet,
	                                  ctx.globalDSet->globalDSets, *ctx.pipelineManager, *ctx.textureManager);
}

static void writeFinalProbeCount(const BakeContext& ctx, int total)
{
	const uint32_t finalCount = static_cast<uint32_t>(1 + total);
	auto cmd = VulkanUtils::beginSingleTimeCommands(*ctx.device);
	cmd.updateBuffer(ctx.bufferManager->buffers[ctx.globalDSet->shProbeCountBuffer.id].buffer[0], 0,
	                 vk::ArrayProxy<const uint32_t>(1, &finalCount));
	VulkanUtils::endSingleTimeCommands(cmd, *ctx.device);
}

static void restoreSkyboxSampler(const BakeContext& ctx, vk::ImageView savedView, vk::Sampler savedSampler)
{
	ctx.descriptorManager->descriptorManager->updateCubemapSamplerDescriptor(*ctx.bindlessDSet, savedView, savedSampler);
}

static void destroyTempImages(const BakeContext& ctx, TempImages& tmp)
{
	tmp.depthView = nullptr; // destroy RAII wrapper before VMA frees the image
	vmaDestroyImage(ctx.allocator, tmp.depthRaw, tmp.depthAlloc);
	tmp.dummyView = nullptr;
	vmaDestroyImage(ctx.allocator, tmp.dummyRaw, tmp.dummyAlloc);
	ctx.textureManager->destroyTexture(tmp.captureHandle);
}

// Entry point
void LightProbeGIBaking::bakeAll(GeneralManager& gm)
{
	BakeContext ctx = gatherContext(gm);

	if (!ctx.grid)
	{
		std::cerr << "[LightProbeGISystem] No LightProbeGridComponent — skipping probe bake.\n";
		return;
	}
	const int totalProbes = ctx.grid->count.x * ctx.grid->count.y * ctx.grid->count.z;
	assert(totalProbes <= static_cast<int>(MAX_SH_PROBES) - 1 && "Probe grid exceeds MAX_SH_PROBES - 1");

	// Save skybox cubemap view/sampler for restoration after baking
	Texture& skyboxTex = ctx.textureManager->textures[ctx.skybox->cubemapTexture.id];
	vk::ImageView savedView = skyboxTex.textureImageView;
	vk::Sampler savedSampler = skyboxTex.textureSampler;

	TempImages tmp = createTempImages(ctx);

	const float influenceRadius = ctx.grid->spacing * 1.2f;
	int probeSlot = 1;

	for (int iz = 0; iz < ctx.grid->count.z; ++iz)
		for (int iy = 0; iy < ctx.grid->count.y; ++iy)
			for (int ix = 0; ix < ctx.grid->count.x; ++ix)
			{
				const glm::vec3 probePos = ctx.grid->origin + ctx.grid->spacing * glm::vec3(ix, iy, iz);
				bakeProbe(ctx, tmp, probeSlot, probePos, influenceRadius);
				++probeSlot;
			}

	writeFinalProbeCount(ctx, totalProbes);
	restoreSkyboxSampler(ctx, savedView, savedSampler);
	destroyTempImages(ctx, tmp);

#ifdef _DEBUG
	std::cout << "[LightProbeGISystem] Baked " << totalProbes << " SH probes.\n";
#endif
}
