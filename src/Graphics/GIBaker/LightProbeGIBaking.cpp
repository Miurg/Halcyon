#include "LightProbeGIBaking.hpp"

static BakeContext gatherContext(GeneralManager& gm)
{
	BakeContext ctx;
	ctx.device = gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	ctx.bufferManager = gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	ctx.textureManager = gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	ctx.modelManager = gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;
	ctx.descriptorManagerComponent = gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	ctx.pipelineManager = gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
	ctx.globalDSet = gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	ctx.modelDSet = gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();
	ctx.bindlessDSet = gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
	ctx.skybox = gm.getContextComponent<SkyBoxContext, SkyboxComponent>();
	ctx.drawInfo = gm.getContextComponent<CurrentFrameContext, DrawInfoComponent>();
	ctx.grid = gm.getContextComponent<LightProbeGridContext, LightProbeGridComponent>();
	ctx.lightComponent = gm.getContextComponent<SunContext, DirectLightComponent>();
	ctx.allocator = gm.getContextComponent<VMAllocatorContext, VMAllocatorComponent>()->allocator;
	ctx.hasSkybox = gm.getContextComponent<SkyBoxContext, SkyboxComponent>()->hasSkybox;
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

	auto cmd = VulkanUtils::beginSingleTimeCommands(*ctx.device);
	VulkanUtils::transitionImageLayout(
	    cmd, depthImageWrap, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal,
	    vk::AccessFlagBits2::eNone, vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
	    vk::PipelineStageFlagBits2::eTopOfPipe, vk::PipelineStageFlagBits2::eEarlyFragmentTests,
	    vk::ImageAspectFlagBits::eDepth, 1, 1);
	VulkanUtils::endSingleTimeCommands(cmd, *ctx.device);

	return tmp;
}

static void writeFinalProbeCount(const BakeContext& ctx, int total)
{
	const uint32_t finalCount = static_cast<uint32_t>(1 + total);
	auto* countPtr =
	    static_cast<uint32_t*>(ctx.bufferManager->buffers[ctx.globalDSet->shProbeCountBuffer.id].bufferMapped[0]);
	*countPtr = finalCount;
}

static void restoreSkyboxSampler(const BakeContext& ctx, vk::ImageView skyboxView, vk::Sampler skyboxSampler)
{
	ctx.descriptorManagerComponent->descriptorManager->updateCubemapSamplerDescriptor(*ctx.bindlessDSet, skyboxView,
	                                                                                  skyboxSampler);
}

static void destroyTempImages(const BakeContext& ctx, TempImages& tmp)
{
	tmp.depthView = nullptr;
	vmaDestroyImage(ctx.allocator, tmp.depthRaw, tmp.depthAlloc);
	ctx.textureManager->destroyTexture(tmp.captureHandle);
}

// Entry point
void LightProbeGIBaking::bakeAll(GeneralManager& gm)
{
	BakeContext ctx = gatherContext(gm);

	if (!ctx.grid)
	{
		std::cerr << "[LightProbeGIBaking] No LightProbeGridComponent — skipping probe bake.\n";
		return;
	}
	const int totalProbes = ctx.grid->count.x * ctx.grid->count.y * ctx.grid->count.z;
	assert(totalProbes <= static_cast<int>(MAX_SH_PROBES) - 1 && "Probe grid exceeds MAX_SH_PROBES - 1");

	// Save skybox cubemap view/sampler for restoration after baking
	Texture& skyboxTex = ctx.textureManager->textures[ctx.skybox->cubemapTexture.id];
	vk::ImageView skyboxView = skyboxTex.textureImageView;
	vk::Sampler skyboxSampler = skyboxTex.textureSampler;

	TempImages tempImages = createTempImages(ctx);

	// Render the shadow map once for the full grid.
	bakeShadowMap(ctx);

	const float influenceRadius = ctx.grid->spacing * 1.2f;
	int probeSlot = 1;

	for (int iz = 0; iz < ctx.grid->count.z; ++iz)
		for (int iy = 0; iy < ctx.grid->count.y; ++iy)
			for (int ix = 0; ix < ctx.grid->count.x; ++ix)
			{
				const glm::vec3 probePos = ctx.grid->origin + ctx.grid->spacing * glm::vec3(ix, iy, iz);
				bakeProbe(ctx, tempImages, probeSlot, probePos, influenceRadius, skyboxView, skyboxSampler);
				++probeSlot;
			}

	writeFinalProbeCount(ctx, totalProbes);
	restoreSkyboxSampler(ctx, skyboxView, skyboxSampler);
	destroyTempImages(ctx, tempImages);

#ifdef _DEBUG
	std::cout << "[LightProbeGISystem] Baked " << totalProbes << " SH probes.\n";
#endif
}
