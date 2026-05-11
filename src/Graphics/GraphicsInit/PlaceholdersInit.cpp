#include "PlaceholdersInit.hpp"
#include <limits>
#include "GraphicsInit.hpp"
#include "../Components/VulkanDeviceComponent.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../Components/VMAllocatorComponent.hpp"
#include "../Components/TextureManagerComponent.hpp"
#include "../Components/BufferManagerComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Components/GraphicsSettingsComponent.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"
#include "../Resources/Components/ModelDSetComponent.hpp"
#include "../Components/SsaoSettingsComponent.hpp"
#include "../Components/RenderGraphComponent.hpp"
#include "../RenderGraph/RenderGraph.hpp"
#include "../Components/NameComponent.hpp"
#include "../Components/CameraComponent.hpp"
#include "../Components/DirectLightComponent.hpp"
#include "../Components/LocalTransformComponent.hpp"
#include "../Components/GlobalTransformComponent.hpp"
#include "../Components/RelationshipComponent.hpp"
#include "../Components/SkyboxComponent.hpp"
#include "../VulkanDevice.hpp"
#include "../SwapChain.hpp"
#include "../Resources/Managers/TextureManager.hpp"
#include "../Resources/Managers/BufferManager.hpp"
#include "../Resources/Managers/ModelManager.hpp"
#include "../Resources/Managers/DescriptorManager.hpp"
#include "../Resources/Factories/TextureUploader.hpp"
#include "../VulkanUtils.hpp"
#include "../GraphicsContexts.hpp"
#include "../Resources/ResourceStructures.hpp"
#include "../Resources/Managers/Bindings.hpp"
#include "../Components/PipelineManagerComponent.hpp"
#include "../Resources/Factories/GltfLoader.hpp"
#include "../Components/LightProbeGridComponent.hpp"

void PlaceholdersInit::initPlaceholders(GeneralManager& gm) 
{
#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::PLACEHOLDERENTITYS::Start creating placeholder entities" << std::endl;
#endif //_DEBUG
#pragma region Fetch Contexts
	DescriptorManager* dManager =
	    gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	BufferManager* bManager = gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	TextureManager* tManager = gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	BindlessTextureDSetComponent* bTextureDSetComponent =
	    gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
	GlobalDSetComponent* globalDSetComponent = gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	SwapChain* swap = gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	VmaAllocator allocator = gm.getContextComponent<VMAllocatorContext, VMAllocatorComponent>()->allocator;
	VulkanDevice* vulkanDevice =
	    gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	PipelineManager& pManager =
	    *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
#pragma endregion

#pragma region Scene Entities (Camera, DirectLight, Skybox)
	// === Camera ===
	Orhescyon::Entity cameraEntity = gm.createEntity();
	gm.addComponent<NameComponent>(cameraEntity, "Main Camera");
	gm.addComponent<CameraComponent>(cameraEntity);
	gm.addComponent<GlobalTransformComponent>(cameraEntity, glm::vec3(-5.0f, 5.0f, 3.0f));
	gm.addComponent<LocalTransformComponent>(cameraEntity, glm::vec3(-5.0f, 5.0f, 3.0f));
	gm.addComponent<RelationshipComponent>(cameraEntity);
	gm.registerContext<MainCameraContext>(cameraEntity);
	CameraComponent* camera = gm.getContextComponent<MainCameraContext, CameraComponent>();

	// === DirectLight ===
	Orhescyon::Entity directLightEntity = gm.createEntity();
	gm.addComponent<NameComponent>(directLightEntity, "Directional Light (Sun)");
	gm.addComponent<CameraComponent>(directLightEntity);
	glm::vec3 directLightPos = glm::vec3(10.0f, 20.0f, 10.0f);
	glm::quat directLightRot = glm::quatLookAt(glm::vec3(0.577f, -0.816f, 0.005f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec4 directLightColor = glm::vec4(0.984f, 1.0f, 0.808f, 50.0f); // Slightly warm white light with high intensity
	glm::vec4 directLightAmbient = directLightColor; // Ambient component is 0.1% of the main light color
	directLightAmbient.w *= 0.001f;
	gm.addComponent<GlobalTransformComponent>(directLightEntity, directLightPos, directLightRot);
	gm.addComponent<LocalTransformComponent>(directLightEntity, directLightPos, directLightRot);
	gm.addComponent<RelationshipComponent>(directLightEntity);
	gm.addComponent<DirectLightComponent>(directLightEntity, 4000, 4000, directLightColor, directLightAmbient);
	gm.registerContext<SunContext>(directLightEntity);
	CameraComponent* directLightCamera = gm.getContextComponent<SunContext, CameraComponent>();
	DirectLightComponent* directLight = gm.getContextComponent<SunContext, DirectLightComponent>();
	directLight->textureShadowImage = tManager->createShadowMap(directLight->sizeX, directLight->sizeY);

#pragma endregion
	// === Graphics Settings ===
	Orhescyon::Entity settingsEntity = gm.createEntity();
	gm.addComponent<NameComponent>(settingsEntity, "Graphics Settings");
	gm.addComponent<GraphicsSettingsComponent>(settingsEntity);
	gm.registerContext<GraphicsSettingsContext>(settingsEntity);

	GraphicsSettingsComponent* settings = gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>();
	vk::SampleCountFlags counts = static_cast<vk::SampleCountFlags>(vulkanDevice->maxMsaaSamples);
	constexpr vk::SampleCountFlagBits kMaxSamples = vk::SampleCountFlagBits::e4;

	// Prefer 4x MSAA if available, but allow fallback to 2x or 1x if not supported
	for (auto samples : {vk::SampleCountFlagBits::e4, vk::SampleCountFlagBits::e2, vk::SampleCountFlagBits::e1})
	{
		if (counts & samples)
		{
			settings->msaaSamples = samples;
			break;
		}
	}
	settings->appliedMsaaSamples = settings->msaaSamples;

#pragma region Global Descriptor Set (Set 0)
	globalDSetComponent->globalDSets =
	    dManager->allocateStorageBufferDSets(MAX_FRAMES_IN_FLIGHT, "globalSet");

	// Camera buffer
	globalDSetComponent->cameraBuffers =
	    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal),
	                           sizeof(CameraStructure), MAX_FRAMES_IN_FLIGHT, vk::BufferUsageFlagBits::eStorageBuffer);
	dManager->updateStorageBufferDescriptors(*bManager, globalDSetComponent->cameraBuffers,
	                                         globalDSetComponent->globalDSets, Bindings::Global::Camera);

	// Sun buffer
	globalDSetComponent->sunCameraBuffers = bManager->createBuffer(
	    (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal),
	    sizeof(DirectLightStructure), MAX_FRAMES_IN_FLIGHT, vk::BufferUsageFlagBits::eStorageBuffer);
	dManager->updateStorageBufferDescriptors(*bManager, globalDSetComponent->sunCameraBuffers,
	                                         globalDSetComponent->globalDSets, Bindings::Global::Sun);
	// Point light buffer
	globalDSetComponent->pointLightBuffers = bManager->createBuffer(
	    (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal),
	    sizeof(PointLightStructure) * 100, MAX_FRAMES_IN_FLIGHT, vk::BufferUsageFlagBits::eStorageBuffer);
	dManager->updateStorageBufferDescriptors(*bManager, globalDSetComponent->pointLightBuffers,
	                                         globalDSetComponent->globalDSets, Bindings::Global::PointLights);
	globalDSetComponent->pointLightCountBuffer =
	    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal),
	                           sizeof(uint32_t), MAX_FRAMES_IN_FLIGHT, vk::BufferUsageFlagBits::eStorageBuffer);
	dManager->updateStorageBufferDescriptors(*bManager, globalDSetComponent->pointLightCountBuffer,
	                                         globalDSetComponent->globalDSets, Bindings::Global::PointLightCount);

	// SH Probe buffer.
	globalDSetComponent->shProbeBuffer =
	    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal),
	                           sizeof(SHProbeEntry) * MAX_SH_PROBES, 1,
	                           vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
	{
		SHProbeEntry skyboxSlot{};
		skyboxSlot.position        = glm::vec3(0.0f);
		skyboxSlot.influenceRadius = std::numeric_limits<float>::max();
		auto cmd = VulkanUtils::beginSingleTimeCommands(*vulkanDevice);
		cmd.updateBuffer(bManager->buffers[globalDSetComponent->shProbeBuffer.id].buffer[0],
		                 0, vk::ArrayProxy<const SHProbeEntry>(1, &skyboxSlot));
		VulkanUtils::endSingleTimeCommands(cmd, *vulkanDevice);
	}
	dManager->updateStorageBufferDescriptors(*bManager, globalDSetComponent->shProbeBuffer,
	                                         globalDSetComponent->globalDSets, Bindings::Global::SHProbes);

	// SH Probe count - slot 0 (skybox) always present, so initial count = 1.
	globalDSetComponent->shProbeCountBuffer =
	    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal),
	                           sizeof(uint32_t), 1,
	                           vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
	dManager->updateStorageBufferDescriptors(*bManager, globalDSetComponent->shProbeCountBuffer,
	                                         globalDSetComponent->globalDSets, Bindings::Global::SHProbeCount);
	{
		uint32_t initialCount = 1u;
		auto cmd = VulkanUtils::beginSingleTimeCommands(*vulkanDevice);
		cmd.updateBuffer(bManager->buffers[globalDSetComponent->shProbeCountBuffer.id].buffer[0],
		                 0, vk::ArrayProxy<const uint32_t>(1, &initialCount));
		VulkanUtils::endSingleTimeCommands(cmd, *vulkanDevice);
	}
#pragma endregion

#pragma region Material & Texture System (Set 2)
	bTextureDSetComponent->bindlessTextureSet = dManager->allocateBindlessTextureDSet();

	// Material Buffer
	bTextureDSetComponent->materialBuffer =
	    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible), 10240 * sizeof(MaterialStructure), 1,
	                           vk::BufferUsageFlagBits::eStorageBuffer);
	dManager->updateStorageBufferDescriptors(*bManager, bTextureDSetComponent->materialBuffer,
	                                         bTextureDSetComponent->bindlessTextureSet, 2);

	// Shadow Map Texture Set (binding 1)
	dManager->updateSingleTextureDSet(bTextureDSetComponent->bindlessTextureSet, 1,
	                                  tManager->textures[directLight->textureShadowImage.id].textureImageView,
	                                  tManager->textures[directLight->textureShadowImage.id].textureSampler);

	// Default White Texture
	auto texturePtr = GltfLoader::createDefaultWhiteTexture();
	int texWidth = texturePtr.get()->width;
	int texHeight = texturePtr.get()->height;
	auto data = texturePtr->pixels.data();
	auto path = texturePtr.get()->name.c_str();
	tManager->generateTextureData(path, texWidth, texHeight, data, *bTextureDSetComponent, *dManager);

	// White placeholder Skybox (can be replaced by SkyboxFactory::loadSkybox)
	Orhescyon::Entity skyboxEntity = gm.createEntity();
	gm.addComponent<NameComponent>(skyboxEntity, "Skybox");
	gm.addComponent<SkyboxComponent>(skyboxEntity);
	gm.registerContext<SkyBoxContext>(skyboxEntity);
	SkyboxComponent* skybox = gm.getContextComponent<SkyBoxContext, SkyboxComponent>();

	TextureHandle whiteCubemapHandle = tManager->createCubemapImage(
	    1, 1, vk::Format::eR32G32B32A32Sfloat,
	    vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eStorage);
	Texture& whiteCubemap = tManager->textures[whiteCubemapHandle.id];
	tManager->createCubemapImageView(whiteCubemap, vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor);
	tManager->createCubemapSampler(whiteCubemap);

	// Transition cubemap to shader-read layout so it's valid for sampling
	{
		auto cmd = VulkanUtils::beginSingleTimeCommands(*vulkanDevice);
		VulkanUtils::transitionImageLayout(
		    cmd, whiteCubemap.textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal, {},
		    vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eTopOfPipe,
		    vk::PipelineStageFlagBits2::eFragmentShader, vk::ImageAspectFlagBits::eColor, 6, 1);
		VulkanUtils::endSingleTimeCommands(cmd, *vulkanDevice);
	}

	dManager->updateCubemapDescriptors(*bTextureDSetComponent, whiteCubemap.textureImageView,
	                                   whiteCubemap.textureSampler, whiteCubemap.textureImageView);

	dManager->updateIBLDescriptors(*bTextureDSetComponent, whiteCubemap.textureImageView, whiteCubemap.textureSampler,
	                               whiteCubemap.textureImageView, whiteCubemap.textureSampler);

	// BRDF LUT - generated once, reused across all skybox changes
	TextureHandle brdfLutHandle = tManager->generateBrdfLut(*dManager, *bTextureDSetComponent, pManager);

	skybox->cubemapTexture = whiteCubemapHandle;
	skybox->prefilteredMap = whiteCubemapHandle;
	skybox->brdfLut = brdfLutHandle;

#pragma endregion

#pragma region Model & Frustum Culling Buffers (Set 1)
	ModelDSetComponent* objectDSetComponent = gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();
	objectDSetComponent->modelBufferDSet =
	    dManager->allocateStorageBufferDSets(MAX_FRAMES_IN_FLIGHT, "modelSet");

	objectDSetComponent->primitiveBuffer =
	    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible), 10240 * sizeof(PrimitiveSctructure),
	                           MAX_FRAMES_IN_FLIGHT, vk::BufferUsageFlagBits::eStorageBuffer);
	dManager->updateStorageBufferDescriptors(*bManager, objectDSetComponent->primitiveBuffer,
	                                         objectDSetComponent->modelBufferDSet, 0);

	objectDSetComponent->transformBuffer =
	    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible), 10240 * sizeof(TransformStructure),
	                           MAX_FRAMES_IN_FLIGHT, vk::BufferUsageFlagBits::eStorageBuffer);
	dManager->updateStorageBufferDescriptors(*bManager, objectDSetComponent->transformBuffer,
	                                         objectDSetComponent->modelBufferDSet, 1);

	objectDSetComponent->indirectDrawBuffer =
	    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal),
	                           sizeof(IndirectDrawStructure) * 10240, MAX_FRAMES_IN_FLIGHT,
	                           vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer |
	                               vk::BufferUsageFlagBits::eTransferDst);
	dManager->updateStorageBufferDescriptors(*bManager, objectDSetComponent->indirectDrawBuffer,
	                                         objectDSetComponent->modelBufferDSet, 2);

	objectDSetComponent->visibleIndicesBuffer =
	    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal),
	                           sizeof(uint32_t) * 10240, MAX_FRAMES_IN_FLIGHT, vk::BufferUsageFlagBits::eStorageBuffer);
	dManager->updateStorageBufferDescriptors(*bManager, objectDSetComponent->visibleIndicesBuffer,
	                                         objectDSetComponent->modelBufferDSet, 3);

	objectDSetComponent->compactedDrawBuffer =
	    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal),
	                           sizeof(IndirectDrawStructure) * 10240, MAX_FRAMES_IN_FLIGHT,
	                           vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer |
	                               vk::BufferUsageFlagBits::eTransferDst);
	dManager->updateStorageBufferDescriptors(*bManager, objectDSetComponent->compactedDrawBuffer,
	                                         objectDSetComponent->modelBufferDSet, 4);

	objectDSetComponent->drawCountBuffer =
	    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal),
	                           sizeof(uint32_t) * 10240, MAX_FRAMES_IN_FLIGHT,
	                           vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer |
	                               vk::BufferUsageFlagBits::eTransferDst);
	dManager->updateStorageBufferDescriptors(*bManager, objectDSetComponent->drawCountBuffer,
	                                         objectDSetComponent->modelBufferDSet, 5);
#pragma endregion

#pragma region Post-Processing & SSAO Descriptor Sets
	RenderGraph* rg = gm.getContextComponent<RenderGraphContext, RenderGraphComponent>()->renderGraph;
	globalDSetComponent->fxaaDSets        = dManager->allocateOffscreenDescriptorSet("screenSpaceSet");
	globalDSetComponent->ssaoDSets        = dManager->allocateOffscreenDescriptorSet("screenSpaceSet");
	globalDSetComponent->ssaoBlurHDSets   = dManager->allocateOffscreenDescriptorSet("screenSpaceSet");
	globalDSetComponent->ssaoBlurVDSets   = dManager->allocateOffscreenDescriptorSet("screenSpaceSet");
	globalDSetComponent->ssaoApplyDSets   = dManager->allocateOffscreenDescriptorSet("screenSpaceSet");
	globalDSetComponent->toneMappingDSets = dManager->allocateOffscreenDescriptorSet("screenSpaceSet");
	globalDSetComponent->vignetteDSets    = dManager->allocateOffscreenDescriptorSet("screenSpaceSet");

	// Bloom descriptor sets (5 downsample + 5 upsample)
	for (int i = 0; i < 5; i++)
	{
		globalDSetComponent->bloomDownsampleDSets[i] =
		    dManager->allocateOffscreenDescriptorSet("screenSpaceSet");
		globalDSetComponent->bloomUpsampleDSets[i] =
		    dManager->allocateOffscreenDescriptorSet("screenSpaceSet");
	}

	// SSAO NoiseInput is a static texture — write it manually.
	tManager->textures.push_back(Texture());
	Texture& noiseTexture = tManager->textures.back();
	tManager->createImage(64, 64, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal,
	                      vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst |
	                          vk::ImageUsageFlagBits::eSampled,
	                      VMA_MEMORY_USAGE_AUTO, noiseTexture, 1);
	tManager->createImageView(noiseTexture, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);

	vk::PhysicalDeviceProperties properties = vulkanDevice->physicalDevice.getProperties();
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = vk::Filter::eNearest;
	samplerInfo.minFilter = vk::Filter::eNearest;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
	samplerInfo.anisotropyEnable = vk::False;
	samplerInfo.compareOp = vk::CompareOp::eAlways;
	samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	noiseTexture.textureSampler = (*vulkanDevice->device).createSampler(samplerInfo);
	int noiseIndex = static_cast<int>(tManager->textures.size() - 1);
	swap->ssaoNoiseTextureHandle.id = noiseIndex;
	TextureUploader::uploadTextureFromFile("assets/textures/LDR_RG01_56.png",
	                                       tManager->textures[swap->ssaoNoiseTextureHandle.id], allocator,
	                                       *vulkanDevice);
	dManager->updateSingleTextureDSet(globalDSetComponent->ssaoDSets, Bindings::SSAO::NoiseInput,
	                                  tManager->textures[swap->ssaoNoiseTextureHandle.id].textureImageView,
	                                  tManager->textures[swap->ssaoNoiseTextureHandle.id].textureSampler);
#pragma endregion

#pragma region SSAO Settings
	Orhescyon::Entity ssaoSettingsEntity = gm.createEntity();
	gm.addComponent<NameComponent>(ssaoSettingsEntity, "SSAO Settings");
	gm.registerContext<SsaoSettingsContext>(ssaoSettingsEntity);
	gm.addComponent<SsaoSettingsComponent>(ssaoSettingsEntity);
#pragma endregion

#pragma region Light Probe Grid
	Orhescyon::Entity probeGridEntity = gm.createEntity();
	gm.registerContext<LightProbeGridContext>(probeGridEntity);
	gm.addComponent<LightProbeGridComponent>(probeGridEntity, LightProbeGridComponent{
	    .origin  = glm::vec3(0.0f),
	    .count   = glm::ivec3(0),
	    .spacing = 0.0f
	});
	gm.addComponent<NameComponent>(probeGridEntity, "SYSTEM Light Probe Grid");
#pragma endregion

#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::PLACEHOLDERENTITYS::Succes!" << std::endl;
#endif //_DEBUG
}
