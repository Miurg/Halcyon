#include "GraphicsCore/GIBaker/ReflectionProbeBaker.hpp"
#include "GraphicsCore/Resources/Factories/EnvMapFactory.hpp"

#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/Components/VulkanDeviceComponent.hpp"
#include "GraphicsCore/Components/BufferManagerComponent.hpp"
#include "GraphicsCore/Components/TextureManagerComponent.hpp"
#include "GraphicsCore/Components/ModelManagerComponent.hpp"
#include "GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include "GraphicsCore/Components/PipelineManagerComponent.hpp"
#include "GraphicsCore/Components/DrawInfoComponent.hpp"
#include "GraphicsCore/Components/SkyboxComponent.hpp"
#include "GraphicsCore/Components/VMAllocatorComponent.hpp"
#include "GraphicsCore/Resources/Components/GlobalDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/ModelDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include "GraphicsCore/Resources/ResourceStructures.hpp"
#include "GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "GraphicsCore/Resources/Managers/Bindings.hpp"
#include "GraphicsCore/Resources/Managers/ModelManager.hpp"
#include "GraphicsCore/Managers/PipelineManager.hpp"
#include "GraphicsCore/Passes/PassCommands.hpp"
#include "GraphicsCore/VulkanUtils.hpp"

#include <array>
#include <cstring>
#include <vector>

namespace
{
constexpr uint32_t kCaptureSize = 256;
constexpr vk::Format kCaptureFormat = vk::Format::eR16G16B16A16Sfloat;

struct RefBakeContext
{
	VulkanDevice* device;
	BufferManager* bufferManager;
	TextureManager* textureManager;
	ModelManager* modelManager;
	DescriptorManagerComponent* descriptorManagerComponent;
	PipelineManager* pipelineManager;
	GlobalDSetComponent* globalDSet;
	ModelDSetComponent* modelDSet;
	BindlessTextureDSetComponent* bindlessDSet;
	SkyboxComponent* skybox;
	DrawInfoComponent* drawInfo;
	VmaAllocator allocator;
	bool hasSkybox;
};

struct RefCapture
{
	TextureHandle cubemap;
	vk::Image image{};
	std::vector<vk::raii::ImageView> faceViews;
	VkImage depthRaw{};
	VmaAllocation depthAlloc{};
	std::vector<vk::raii::ImageView> depthViews;
};

struct BakeFacePush
{
	glm::vec3 probePos;
	uint32_t faceIdx;
};

RefBakeContext gather(GeneralManager& gm)
{
	RefBakeContext c;
	c.device = gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	c.bufferManager = gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	c.textureManager = gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	c.modelManager = gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;
	c.descriptorManagerComponent = gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	c.pipelineManager = gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	c.globalDSet = gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	c.modelDSet = gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();
	c.bindlessDSet = gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
	c.skybox = gm.getContextComponent<SkyBoxContext, SkyboxComponent>();
	c.drawInfo = gm.getContextComponent<CurrentFrameContext, DrawInfoComponent>();
	c.allocator = gm.getContextComponent<VMAllocatorContext, VMAllocatorComponent>()->allocator;
	c.hasSkybox = c.skybox->hasSkybox;
	return c;
}

RefCapture createCapture(const RefBakeContext& ctx)
{
	RefCapture cap;
	cap.cubemap = ctx.textureManager->createCubemapImage(
	    kCaptureSize, kCaptureSize, kCaptureFormat,
	    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
	Texture& tex = ctx.textureManager->getTexture(cap.cubemap);
	cap.image = tex.textureImage;
	ctx.textureManager->createCubemapImageView(tex, kCaptureFormat, vk::ImageAspectFlagBits::eColor);
	ctx.textureManager->createCubemapSampler(tex);

	cap.faceViews.reserve(6);
	for (uint32_t f = 0; f < 6; ++f)
		cap.faceViews.push_back(VulkanUtils::createImageView(cap.image, kCaptureFormat,
		                                                     vk::ImageAspectFlagBits::eColor, *ctx.device,
		                                                     vk::ImageViewType::e2D, 1, 0, 1, f));

	const vk::Format depthFormat = ctx.textureManager->findBestFormat();
	VkImageCreateInfo depthCI{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	depthCI.imageType = VK_IMAGE_TYPE_2D;
	depthCI.format = static_cast<VkFormat>(depthFormat);
	depthCI.extent = {kCaptureSize, kCaptureSize, 1};
	depthCI.mipLevels = 1;
	depthCI.arrayLayers = 6;
	depthCI.samples = VK_SAMPLE_COUNT_1_BIT;
	depthCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	depthCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	depthCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VmaAllocationCreateInfo depthAllocCI{};
	depthAllocCI.usage = VMA_MEMORY_USAGE_AUTO;
	if (vmaCreateImage(ctx.allocator, &depthCI, &depthAllocCI, &cap.depthRaw, &cap.depthAlloc, nullptr) != VK_SUCCESS)
		throw std::runtime_error("[ReflectionProbeBaker] Failed to create temp depth image.");

	vk::Image depthImage(cap.depthRaw);
	cap.depthViews.reserve(6);
	for (uint32_t f = 0; f < 6; ++f)
		cap.depthViews.push_back(VulkanUtils::createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth,
		                                                      *ctx.device, vk::ImageViewType::e2D, 1, 0, 1, f));

	auto cmd = VulkanUtils::beginSingleTimeCommands(*ctx.device);
	VulkanUtils::transitionImageLayout(cmd, depthImage, vk::ImageLayout::eUndefined,
	                                   vk::ImageLayout::eDepthAttachmentOptimal, vk::AccessFlagBits2::eNone,
	                                   vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
	                                   vk::PipelineStageFlagBits2::eTopOfPipe,
	                                   vk::PipelineStageFlagBits2::eEarlyFragmentTests, vk::ImageAspectFlagBits::eDepth,
	                                   6, 1);
	VulkanUtils::endSingleTimeCommands(cmd, *ctx.device);
	return cap;
}

void destroyCapture(const RefBakeContext& ctx, RefCapture& cap)
{
	cap.faceViews.clear();
	cap.depthViews.clear();
	vmaDestroyImage(ctx.allocator, cap.depthRaw, cap.depthAlloc);
	ctx.textureManager->destroyTexture(cap.cubemap);
}

void drawScene(vk::raii::CommandBuffer& cmd, const RefBakeContext& ctx, glm::vec3 origin, int faceIdx)
{
	const uint32_t stride = sizeof(VkDrawIndexedIndirectCommand);
	const BakeFacePush push{origin, static_cast<uint32_t>(faceIdx)};
	DescriptorManager& dm = *ctx.descriptorManagerComponent->descriptorManager;

	cmd.bindVertexBuffers(0, ctx.modelManager->getVertexIndexBuffer(0).vertexBuffer, {0});
	cmd.bindIndexBuffer(ctx.modelManager->getVertexIndexBuffer(0).indexBuffer, 0, vk::IndexType::eUint32);

	auto bindMain = [&](const char* pipe)
	{
		auto& p = ctx.pipelineManager->pipelines[pipe];
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *p.pipeline);
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *p.layout, 0, dm.getSet(ctx.globalDSet->globalDSets, 0),
		                       nullptr);
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *p.layout, 1,
		                       dm.getSet(ctx.modelDSet->modelBufferDSet, 0), nullptr);
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *p.layout, 2,
		                       dm.getSet(ctx.bindlessDSet->bindlessTextureSet), nullptr);
		cmd.pushConstants<BakeFacePush>(*p.layout, vk::ShaderStageFlagBits::eVertex, 0, push);
	};

	const uint32_t segCounts[6] = {ctx.drawInfo->opaqueSingleCount, ctx.drawInfo->opaqueDoubleCount,
	                               ctx.drawInfo->maskSingleCount,   ctx.drawInfo->maskDoubleCount,
	                               ctx.drawInfo->blendSingleCount,  ctx.drawInfo->blendDoubleCount};
	uint32_t cmdOffset = 0;
	uint32_t countOffset = 0;
	auto drawSegment = [&](uint32_t seg)
	{
		uint32_t count = segCounts[seg];
		if (count > 0)
		{
			cmd.setCullMode((seg & 1) ? vk::CullModeFlagBits::eNone : vk::CullModeFlagBits::eBack);
			cmd.drawIndexedIndirectCount(ctx.bufferManager->getBuffer(ctx.modelDSet->compactedDrawBuffer),
			                             cmdOffset,
			                             ctx.bufferManager->getBuffer(ctx.modelDSet->drawCountBuffer),
			                             countOffset, count, stride);
			cmdOffset += count * stride;
		}
		countOffset += sizeof(uint32_t);
	};

	bindMain("global_illumination_forward");
	drawSegment(0);
	drawSegment(1);

	if (ctx.hasSkybox)
	{
		auto& sky = ctx.pipelineManager->pipelines["skybox_capture"];
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *sky.pipeline);
		cmd.pushConstants<BakeFacePush>(*sky.layout, vk::ShaderStageFlagBits::eVertex, 0, push);
		cmd.setCullMode(vk::CullModeFlagBits::eNone);
		cmd.draw(3, 1, 0, 0);
	}

	bindMain("global_illumination_forward_alpha");
	drawSegment(2);
	drawSegment(3);
	drawSegment(4);
	drawSegment(5);

	const uint32_t lightCount = *ctx.bufferManager->getMapped<uint32_t>(ctx.globalDSet->pointLightCountBuffer);
	if (lightCount > 0)
	{
		struct LightSourcePush
		{
			glm::vec3 probePos;
			float scale;
			uint32_t faceIdx;
		};
		const LightSourcePush lpush{origin, 0.15f, static_cast<uint32_t>(faceIdx)};
		auto& p = ctx.pipelineManager->pipelines["gi_light_source_bake"];
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *p.pipeline);
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *p.layout, 0, dm.getSet(ctx.globalDSet->globalDSets, 0),
		                       nullptr);
		cmd.setCullMode(vk::CullModeFlagBits::eBack);
		cmd.pushConstants<LightSourcePush>(*p.layout, vk::ShaderStageFlagBits::eVertex, 0, lpush);
		cmd.draw(384u, lightCount, 0u, 0u);
	}
}

void wholeImageBarrier(vk::raii::CommandBuffer& cmd, vk::Image image, vk::ImageLayout oldL, vk::ImageLayout newL,
                       vk::AccessFlags2 src, vk::AccessFlags2 dst, vk::PipelineStageFlags2 srcS,
                       vk::PipelineStageFlags2 dstS)
{
	vk::ImageMemoryBarrier2 b;
	b.srcStageMask = srcS;
	b.srcAccessMask = src;
	b.dstStageMask = dstS;
	b.dstAccessMask = dst;
	b.oldLayout = oldL;
	b.newLayout = newL;
	b.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
	b.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
	b.image = image;
	b.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 6};
	vk::DependencyInfo dep;
	dep.imageMemoryBarrierCount = 1;
	dep.pImageMemoryBarriers = &b;
	cmd.pipelineBarrier2(dep);
}
} // namespace

void ReflectionProbeBaker::bake(GeneralManager& gm, ReflectionProbeComponent& probe, int cubemapSlot)
{
	RefBakeContext ctx = gather(gm);
	if (ctx.modelManager->meshes.empty()) return;

	ctx.device->device.waitIdle();

	auto* gridInfo = ctx.bufferManager->getMapped<SHGridInfo>(ctx.globalDSet->shGridInfoBuffer);
	const float savedRange = gridInfo->captureRange;
	const glm::vec3 savedAmbient = gridInfo->giAmbient;
	const float savedBounce = gridInfo->giBounceMultiplier;
	gridInfo->captureRange = probe.captureRange;
	gridInfo->giAmbient = probe.giAmbientColor * probe.giAmbientIntensity;
	gridInfo->giBounceMultiplier = probe.giBounceMultiplier;

	// All-accepting frustum in the main camera buffer so the cull passes the whole scene; save/restore it.
	CameraStructure savedCam;
	std::memcpy(&savedCam, ctx.bufferManager->getMapped<CameraStructure>(ctx.globalDSet->cameraBuffers),
	            sizeof(CameraStructure));
	CameraStructure infiniteCam{};
	for (int i = 0; i < 6; ++i) infiniteCam.frustumPlanes[i] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	std::memcpy(ctx.bufferManager->getMapped<CameraStructure>(ctx.globalDSet->cameraBuffers), &infiniteCam,
	            sizeof(CameraStructure));

	RefCapture cap = createCapture(ctx);

	auto cmd = VulkanUtils::beginSingleTimeCommands(*ctx.device);

	drawResetInstancePass(cmd, 0, *ctx.descriptorManagerComponent, *ctx.modelDSet, *ctx.drawInfo, *ctx.pipelineManager);
	drawCullPass(cmd, 0, *ctx.descriptorManagerComponent, *ctx.globalDSet, *ctx.modelDSet, *ctx.modelManager,
	             *ctx.bufferManager, *ctx.drawInfo, *ctx.pipelineManager);
	{
		vk::MemoryBarrier2 b;
		b.srcStageMask = vk::PipelineStageFlagBits2::eComputeShader;
		b.srcAccessMask = vk::AccessFlagBits2::eShaderWrite;
		b.dstStageMask = vk::PipelineStageFlagBits2::eDrawIndirect | vk::PipelineStageFlagBits2::eVertexShader;
		b.dstAccessMask = vk::AccessFlagBits2::eIndirectCommandRead | vk::AccessFlagBits2::eShaderRead;
		vk::DependencyInfo dep;
		dep.memoryBarrierCount = 1;
		dep.pMemoryBarriers = &b;
		cmd.pipelineBarrier2(dep);
	}

	wholeImageBarrier(cmd, cap.image, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
	                  vk::AccessFlagBits2::eNone, vk::AccessFlagBits2::eColorAttachmentWrite,
	                  vk::PipelineStageFlagBits2::eTopOfPipe, vk::PipelineStageFlagBits2::eColorAttachmentOutput);

	for (int face = 0; face < 6; ++face)
	{
		vk::RenderingAttachmentInfo colorAtt;
		colorAtt.imageView = *cap.faceViews[face];
		colorAtt.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		colorAtt.loadOp = vk::AttachmentLoadOp::eClear;
		colorAtt.storeOp = vk::AttachmentStoreOp::eStore;
		colorAtt.clearValue = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);

		vk::RenderingAttachmentInfo depthAtt;
		depthAtt.imageView = *cap.depthViews[face];
		depthAtt.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
		depthAtt.loadOp = vk::AttachmentLoadOp::eClear;
		depthAtt.storeOp = vk::AttachmentStoreOp::eDontCare;
		depthAtt.clearValue = vk::ClearDepthStencilValue(0.0f, 0);

		vk::RenderingInfo ri;
		ri.renderArea = vk::Rect2D{{0, 0}, {kCaptureSize, kCaptureSize}};
		ri.layerCount = 1;
		ri.colorAttachmentCount = 1;
		ri.pColorAttachments = &colorAtt;
		ri.pDepthAttachment = &depthAtt;

		cmd.beginRendering(ri);
		cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, float(kCaptureSize), float(kCaptureSize), 0.0f, 1.0f));
		cmd.setScissor(0, vk::Rect2D({0, 0}, {kCaptureSize, kCaptureSize}));
		drawScene(cmd, ctx, probe.origin, face);
		cmd.endRendering();
	}

	wholeImageBarrier(cmd, cap.image, vk::ImageLayout::eColorAttachmentOptimal,
	                  vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits2::eColorAttachmentWrite,
	                  vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eColorAttachmentOutput,
	                  vk::PipelineStageFlagBits2::eFragmentShader);

	VulkanUtils::endSingleTimeCommands(cmd, *ctx.device);

	// Prefilter the capture into a persistent GGX cubemap and bind it into the reflection array slot.
	if (probe.prefilteredMap.id >= 0) ctx.textureManager->destroyTexture(probe.prefilteredMap);
	TextureHandle prefiltered = EnvMapFactory::prefilteredEnvMap(
	    *ctx.textureManager, *ctx.device, cap.cubemap, *ctx.descriptorManagerComponent->descriptorManager,
	    *ctx.bindlessDSet, *ctx.pipelineManager);

	ctx.descriptorManagerComponent->descriptorManager->update(
	    ctx.bindlessDSet->bindlessTextureSet, Bindings::Textures::ReflectionCubemaps, 0,
	    vk::DescriptorType::eCombinedImageSampler, ctx.textureManager->getTexture(prefiltered).textureImageView,
	    ctx.textureManager->getTexture(prefiltered).textureSampler, vk::ImageLayout::eShaderReadOnlyOptimal,
	    static_cast<uint32_t>(cubemapSlot));

	probe.prefilteredMap = prefiltered;
	probe.cubemapIndex = cubemapSlot;

	ctx.device->device.waitIdle();
	destroyCapture(ctx, cap);

	// Restore the skybox cubemap sampler (generatePrefilteredEnvMap left it pointing at the capture)
	// and the main camera buffer / grid capture range.
	Texture& skyTex = ctx.textureManager->getTexture(ctx.skybox->cubemapTexture);
	ctx.descriptorManagerComponent->descriptorManager->update(
	    ctx.bindlessDSet->bindlessTextureSet, Bindings::Textures::CubemapSampler, 0,
	    vk::DescriptorType::eCombinedImageSampler, skyTex.textureImageView, skyTex.textureSampler);
	std::memcpy(ctx.bufferManager->getMapped<CameraStructure>(ctx.globalDSet->cameraBuffers), &savedCam,
	            sizeof(CameraStructure));
	gridInfo->captureRange = savedRange;
	gridInfo->giAmbient = savedAmbient;
	gridInfo->giBounceMultiplier = savedBounce;
}
