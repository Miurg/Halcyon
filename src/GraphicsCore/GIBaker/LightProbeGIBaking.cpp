#include "LightProbeGIBaking.hpp"
#include "GraphicsCore/Resources/Factories/TextureFactory.hpp"

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
	tmp.captureHandle = TextureFactory::createTexture(
	    *ctx.textureManager,
	    imagePresets::cubemap(kCaptureSize, kCaptureFormat,
	                          vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled),
	    samplerPresets::cubemap(), vk::ImageAspectFlagBits::eColor, vk::ImageViewType::eCube);

	Texture& captureTex = ctx.textureManager->getTexture(tmp.captureHandle);
	tmp.captureImage = captureTex.textureImage;

	tmp.faceViews.reserve(6);
	for (uint32_t face = 0; face < 6; ++face)
		tmp.faceViews.push_back(VulkanUtils::createImageView(tmp.captureImage, kCaptureFormat,
		                                                     vk::ImageAspectFlagBits::eColor, *ctx.device,
		                                                     vk::ImageViewType::e2D, 1, 0, 1, face));

	// Depth image (VMA-owned)
	const vk::Format depthFormat = ctx.textureManager->findBestFormat();

	ImageDesc depthDesc;
	depthDesc.width = kCaptureSize;
	depthDesc.height = kCaptureSize;
	depthDesc.format = depthFormat;
	depthDesc.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
	depthDesc.layerCount = 6;
	AllocatedImage depthAllocated = VulkanUtils::createImage(ctx.allocator, depthDesc);
	tmp.depthRaw = depthAllocated.image;
	tmp.depthAlloc = depthAllocated.allocation;

	vk::Image depthImageWrap(tmp.depthRaw);
	tmp.depthViews.reserve(6);
	for (uint32_t face = 0; face < 6; ++face)
		tmp.depthViews.push_back(VulkanUtils::createImageView(depthImageWrap, depthFormat,
		                                                      vk::ImageAspectFlagBits::eDepth, *ctx.device,
		                                                      vk::ImageViewType::e2D, 1, 0, 1, face));

	auto cmd = VulkanUtils::beginSingleTimeCommands(*ctx.device);
	VulkanUtils::transitionImageLayout(
	    cmd, depthImageWrap, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal,
	    vk::AccessFlagBits2::eNone, vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
	    vk::PipelineStageFlagBits2::eTopOfPipe, vk::PipelineStageFlagBits2::eEarlyFragmentTests,
	    vk::ImageAspectFlagBits::eDepth, 6, 1);
	VulkanUtils::endSingleTimeCommands(cmd, *ctx.device);

	return tmp;
}

static void writeGridInfo(const BakeContext& ctx, int total)
{
	auto* gridInfo = ctx.bufferManager->getMapped<SHGridInfo>(ctx.globalDSet->shGridInfoBuffer);
	gridInfo->origin = ctx.grid->origin;
	gridInfo->spacing = ctx.grid->spacing;
	gridInfo->count = ctx.grid->count;
	gridInfo->probeCount = static_cast<uint32_t>(1 + total);
	gridInfo->captureRange = ctx.grid->captureRange;
	gridInfo->giAmbient = ctx.grid->giAmbientColor * ctx.grid->giAmbientIntensity;
	gridInfo->giBounceMultiplier = ctx.grid->giBounceMultiplier;
}

static void destroyTempImages(const BakeContext& ctx, TempImages& tmp)
{
	tmp.faceViews.clear();
	tmp.depthViews.clear();
	vmaDestroyImage(ctx.allocator, tmp.depthRaw, tmp.depthAlloc);
	ctx.textureManager->destroyTexture(tmp.captureHandle);
}

// Region capacities match the main model buffers (10240 draw commands / objects).
static constexpr uint32_t kBakeRegionCount = static_cast<uint32_t>(kProbesPerSubmit) * 6u;
static constexpr uint32_t kBakeMaxDrawCommands = 10240;

static void ensureBakeBuffers(const BakeContext& ctx)
{
	if (ctx.modelDSet->bakeBuffersReady) return;

	BufferManager& bufferManager = *ctx.bufferManager;
	const auto memoryProps = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal;

	ctx.modelDSet->bakeIndirectDrawBuffer =
	    bufferManager.createBuffer(memoryProps, sizeof(IndirectDrawStructure) * kBakeMaxDrawCommands * kBakeRegionCount, 1,
	                          vk::BufferUsageFlagBits::eStorageBuffer);
	ctx.modelDSet->bakeVisibleIndicesBuffer =
	    bufferManager.createBuffer(memoryProps, sizeof(uint32_t) * kBakeMaxDrawCommands * kBakeRegionCount, 1,
	                          vk::BufferUsageFlagBits::eStorageBuffer);
	ctx.modelDSet->bakeCompactedDrawBuffer =
	    bufferManager.createBuffer(memoryProps, sizeof(IndirectDrawStructure) * kBakeMaxDrawCommands * kBakeRegionCount, 1,
	                          vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer);
	ctx.modelDSet->bakeDrawCountBuffer =
	    bufferManager.createBuffer(memoryProps, sizeof(uint32_t) * 6 * kBakeRegionCount, 1,
	                          vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer);

	DescriptorManager& descriptorManager = *ctx.descriptorManagerComponent->descriptorManager;
	descriptorManager.updateStorageBufferDescriptors(bufferManager, ctx.modelDSet->primitiveBuffer, ctx.modelDSet->bakeModelDSet, 0);
	descriptorManager.updateStorageBufferDescriptors(bufferManager, ctx.modelDSet->transformBuffer, ctx.modelDSet->bakeModelDSet, 1);
	descriptorManager.updateStorageBufferDescriptors(bufferManager, ctx.modelDSet->bakeIndirectDrawBuffer,
	                                        ctx.modelDSet->bakeModelDSet, 2);
	descriptorManager.updateStorageBufferDescriptors(bufferManager, ctx.modelDSet->bakeVisibleIndicesBuffer,
	                                        ctx.modelDSet->bakeModelDSet, 3);
	descriptorManager.updateStorageBufferDescriptors(bufferManager, ctx.modelDSet->bakeCompactedDrawBuffer,
	                                        ctx.modelDSet->bakeModelDSet, 4);
	descriptorManager.updateStorageBufferDescriptors(bufferManager, ctx.modelDSet->bakeDrawCountBuffer, ctx.modelDSet->bakeModelDSet,
	                                        5);

	ctx.modelDSet->bakeBuffersReady = true;
}

static void computeToComputeBarrier(vk::raii::CommandBuffer& cmd)
{
	vk::MemoryBarrier2 barrier;
	barrier.srcStageMask = vk::PipelineStageFlagBits2::eComputeShader;
	barrier.srcAccessMask = vk::AccessFlagBits2::eShaderWrite;
	barrier.dstStageMask = vk::PipelineStageFlagBits2::eComputeShader;
	barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite;
	vk::DependencyInfo depInfo;
	depInfo.memoryBarrierCount = 1;
	depInfo.pMemoryBarriers = &barrier;
	cmd.pipelineBarrier2(depInfo);
}

// Culls the whole chunk in one go: one thread per (object, probe-face region).
static void recordChunkCull(vk::raii::CommandBuffer& cmd, const BakeContext& ctx, uint32_t firstProbeLinear,
                            uint32_t probesInChunk)
{
	DescriptorManager& descriptorManager = *ctx.descriptorManagerComponent->descriptorManager;
	const uint32_t regions = probesInChunk * 6u;
	const uint32_t drawCount = ctx.drawInfo->totalDrawCount;
	const uint32_t objectCount = ctx.drawInfo->totalObjectCount;
	if (drawCount == 0 || objectCount == 0) return;

	{
		auto& pip = ctx.pipelineManager->pipelines["gi_bake_reset"];
		cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pip.pipeline);
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pip.layout, 0,
		                       descriptorManager.getSet(ctx.modelDSet->modelBufferDSet, 0), nullptr);
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pip.layout, 1,
		                       descriptorManager.getSet(ctx.modelDSet->bakeModelDSet), nullptr);
		struct ResetPush
		{
			uint32_t drawCommandCount;
			uint32_t visibleCapacity;
		};
		const ResetPush push{drawCount, objectCount};
		cmd.pushConstants<ResetPush>(*pip.layout, vk::ShaderStageFlagBits::eCompute, 0, push);
		cmd.dispatch((drawCount + 63) / 64, regions, 1);
	}

	computeToComputeBarrier(cmd);

	{
		auto& pip = ctx.pipelineManager->pipelines["gi_bake_cull"];
		cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pip.pipeline);
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pip.layout, 0,
		                       descriptorManager.getSet(ctx.globalDSet->globalDSets, 0), nullptr);
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pip.layout, 1,
		                       descriptorManager.getSet(ctx.modelDSet->bakeModelDSet), nullptr);
		struct CullPush
		{
			uint32_t objectCount;
			uint32_t drawCommandCount;
			uint32_t firstProbeLinear;
		};
		const CullPush push{objectCount, drawCount, firstProbeLinear};
		cmd.pushConstants<CullPush>(*pip.layout, vk::ShaderStageFlagBits::eCompute, 0, push);
		cmd.dispatch((objectCount + 63) / 64, regions, 1);
	}

	computeToComputeBarrier(cmd);

	{
		auto& pip = ctx.pipelineManager->pipelines["gi_bake_compaction"];
		cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pip.pipeline);
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pip.layout, 0,
		                       descriptorManager.getSet(ctx.modelDSet->bakeModelDSet), nullptr);
		struct CompactionPush
		{
			uint32_t drawCommandCount;
			uint32_t segStart1;
			uint32_t segStart2;
			uint32_t segStart3;
			uint32_t segStart4;
			uint32_t segStart5;
		};
		CompactionPush push;
		push.drawCommandCount = drawCount;
		push.segStart1 = ctx.drawInfo->opaqueSingleCount;
		push.segStart2 = push.segStart1 + ctx.drawInfo->opaqueDoubleCount;
		push.segStart3 = push.segStart2 + ctx.drawInfo->maskSingleCount;
		push.segStart4 = push.segStart3 + ctx.drawInfo->maskDoubleCount;
		push.segStart5 = push.segStart4 + ctx.drawInfo->blendSingleCount;
		cmd.pushConstants<CompactionPush>(*pip.layout, vk::ShaderStageFlagBits::eCompute, 0, push);
		cmd.dispatch((drawCount + 63) / 64, regions, 1);
	}

	// Cull outputs -> indirect draws + vertex reads for every render in the chunk
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
}

void LightProbeGIBaking::resetProbes(GeneralManager& gm)
{
	BakeContext ctx = gatherContext(gm);
	ctx.device->device.waitIdle();

	auto* probes = ctx.bufferManager->getMapped<SHProbeEntry>(ctx.globalDSet->shProbeBuffer);
	// Slot 0 is the skybox fallback (owned by SkyboxFactory), not a bake result — keep it
	std::memset(probes + 1, 0, sizeof(SHProbeEntry) * (MAX_SH_PROBES - 1));
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

	ctx.device->device.waitIdle();
	TempImages tempImages = createTempImages(ctx);

	// Render the shadow map once for the full grid.
	bakeShadowMap(ctx);

	ensureBakeBuffers(ctx);

	// gi_bake_cull derives probe positions from shGridInfo, so it must be written before recording.
	writeGridInfo(ctx, totalProbes);

	// Point sh_projection at the capture cubemap once for the whole bake.
	Texture& captureTex = ctx.textureManager->getTexture(tempImages.captureHandle);
	ctx.descriptorManagerComponent->descriptorManager->update(
	    ctx.bindlessDSet->bindlessTextureSet, Bindings::Textures::GICaptureCubemap, 0,
	    vk::DescriptorType::eCombinedImageSampler, captureTex.textureImageView,
	    ctx.textureManager->getSampler(captureTex.samplerHandle));

	const float influenceRadius = ctx.grid->spacing * 1.2f;
	const glm::ivec3 gridCount = ctx.grid->count;

	for (int chunkStart = 0; chunkStart < totalProbes; chunkStart += kProbesPerSubmit)
	{
		const int probesInChunk = std::min(kProbesPerSubmit, totalProbes - chunkStart);

		auto cmd = VulkanUtils::beginSingleTimeCommands(*ctx.device);
		recordChunkCull(cmd, ctx, static_cast<uint32_t>(chunkStart), static_cast<uint32_t>(probesInChunk));

		for (int i = 0; i < probesInChunk; ++i)
		{
			// Linear order matches the slot layout: x fastest, slot 0 = skybox
			const int linear = chunkStart + i;
			const int ix = linear % gridCount.x;
			const int iy = (linear / gridCount.x) % gridCount.y;
			const int iz = linear / (gridCount.x * gridCount.y);
			const glm::vec3 probePos = ctx.grid->origin + ctx.grid->spacing * glm::vec3(ix, iy, iz);
			recordProbe(cmd, ctx, tempImages, static_cast<uint32_t>(i) * 6u, 1 + linear, probePos, influenceRadius);
		}
		VulkanUtils::endSingleTimeCommands(cmd, *ctx.device);
	}

	// Re-point the capture binding at the skybox before the capture cubemap is destroyed.
	Texture& skyboxTex = ctx.textureManager->getTexture(ctx.skybox->cubemapTexture);
	ctx.descriptorManagerComponent->descriptorManager->update(
	    ctx.bindlessDSet->bindlessTextureSet, Bindings::Textures::GICaptureCubemap, 0,
	    vk::DescriptorType::eCombinedImageSampler, skyboxTex.textureImageView,
	    ctx.textureManager->getSampler(skyboxTex.samplerHandle));

	// All submissions must finish before we free temp cubemap/depth.
	ctx.device->device.waitIdle();
	destroyTempImages(ctx, tempImages);

#ifdef _DEBUG
	std::cout << "[LightProbeGISystem] Baked " << totalProbes << " SH probes.\n";
#endif
}
