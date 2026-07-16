#include "PlaceholdersInit.hpp"
#include <limits>
#include "GraphicsInit.hpp"
#include "GraphicsCore/Components/VulkanDeviceComponent.hpp"
#include "GraphicsCore/Components/TextureManagerComponent.hpp"
#include "GraphicsCore/Components/BufferManagerComponent.hpp"
#include "GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include "GraphicsCore/Components/GraphicsSettingsComponent.hpp"
#include "GraphicsCore/Resources/Components/GlobalDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/ModelDSetComponent.hpp"
#include "GraphicsCore/Components/GtaoSettingsComponent.hpp"
#include "GraphicsCore/Components/AutoExposureSettingsComponent.hpp"
#include "GraphicsCore/Components/NameComponent.hpp"
#include "GraphicsCore/Components/CameraComponent.hpp"
#include "GraphicsCore/Components/DirectLightComponent.hpp"
#include "GraphicsCore/Components/LocalTransformComponent.hpp"
#include "GraphicsCore/Components/GlobalTransformComponent.hpp"
#include "GraphicsCore/Components/RelationshipComponent.hpp"
#include "GraphicsCore/Components/SkyboxComponent.hpp"
#include "GraphicsCore/VulkanDevice.hpp"
#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "GraphicsCore/Resources/Managers/ModelManager.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "GraphicsCore/VulkanUtils.hpp"
#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/Resources/ResourceStructures.hpp"
#include "GraphicsCore/Resources/Managers/Bindings.hpp"
#include "GraphicsCore/Components/PipelineManagerComponent.hpp"
#include "../Resources/Factories/GltfLoader.hpp"
#include "GraphicsCore/Resources/Factories/EnvMapFactory.hpp"
#include "GraphicsCore/Components/LightProbeGridComponent.hpp"
#include "GraphicsCore/Components/DeltaTimeComponent.hpp"
#include "GraphicsCore/Systems/TransformSystem.hpp"

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
	gm.addComponent<GlobalTransformComponent>(cameraEntity, glm::vec3(0.0f, 0.0f, 0.0f));
	gm.addComponent<LocalTransformComponent>(cameraEntity, glm::vec3(0.0f, 0.0f, 0.0f));
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
	glm::vec4 directLightAmbient = directLightColor;
	directLightAmbient.w *= 0.0f;
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
	gm.addComponent<AutoExposureSettingsComponent>(settingsEntity);
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
	globalDSetComponent->globalDSets = dManager->allocate("globalSet", MAX_FRAMES_IN_FLIGHT);

	// Camera buffer
	globalDSetComponent->cameraBuffers =
	    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal),
	                           sizeof(CameraStructure), MAX_FRAMES_IN_FLIGHT,
	                           vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
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
		skyboxSlot.position = glm::vec3(0.0f);
		skyboxSlot.influenceRadius = std::numeric_limits<float>::max();
		auto cmd = VulkanUtils::beginSingleTimeCommands(*vulkanDevice);
		cmd.updateBuffer(bManager->getBuffer(globalDSetComponent->shProbeBuffer), 0,
		                 vk::ArrayProxy<const SHProbeEntry>(1, &skyboxSlot));
		VulkanUtils::endSingleTimeCommands(cmd, *vulkanDevice);
	}
	dManager->updateStorageBufferDescriptors(*bManager, globalDSetComponent->shProbeBuffer,
	                                         globalDSetComponent->globalDSets, Bindings::Global::SHProbes);

	// SH grid info - slot 0 (skybox) always present, so initial probeCount = 1.
	globalDSetComponent->shGridInfoBuffer = bManager->createBuffer(
	    (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal), sizeof(SHGridInfo), 1,
	    vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
	dManager->updateStorageBufferDescriptors(*bManager, globalDSetComponent->shGridInfoBuffer,
	                                         globalDSetComponent->globalDSets, Bindings::Global::SHGridInfo);
	{
		SHGridInfo initialGridInfo{};
		initialGridInfo.probeCount = 1u;
		initialGridInfo.giBounceMultiplier = 1.0f;
		auto cmd = VulkanUtils::beginSingleTimeCommands(*vulkanDevice);
		cmd.updateBuffer(bManager->getBuffer(globalDSetComponent->shGridInfoBuffer), 0,
		                 vk::ArrayProxy<const SHGridInfo>(1, &initialGridInfo));
		VulkanUtils::endSingleTimeCommands(cmd, *vulkanDevice);
	}

	// Reflection probes — box metadata + cubemap indices, refilled per frame by ReflectionProbeUpdateSystem.
	globalDSetComponent->reflectionProbeBuffer = bManager->createBuffer(
	    (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal),
	    sizeof(ReflectionProbeData) * MAX_REFLECTION_PROBES, MAX_FRAMES_IN_FLIGHT,
	    vk::BufferUsageFlagBits::eStorageBuffer);
	dManager->updateStorageBufferDescriptors(*bManager, globalDSetComponent->reflectionProbeBuffer,
	                                         globalDSetComponent->globalDSets, Bindings::Global::ReflectionProbes);

	globalDSetComponent->reflectionProbeCountBuffer = bManager->createBuffer(
	    (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal), sizeof(uint32_t),
	    MAX_FRAMES_IN_FLIGHT, vk::BufferUsageFlagBits::eStorageBuffer);
	dManager->updateStorageBufferDescriptors(*bManager, globalDSetComponent->reflectionProbeCountBuffer,
	                                         globalDSetComponent->globalDSets, Bindings::Global::ReflectionProbeCount);
	for (uint32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; ++frame)
		*bManager->getMapped<uint32_t>(globalDSetComponent->reflectionProbeCountBuffer, frame) = 0u;
#pragma endregion

#pragma region Material & Texture System (Set 2)
	bTextureDSetComponent->bindlessTextureSet = dManager->allocate("textureSet");

	// Material Buffer
	bTextureDSetComponent->materialBuffer =
	    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible), 10240 * sizeof(MaterialStructure), 1,
	                           vk::BufferUsageFlagBits::eStorageBuffer);
	dManager->updateStorageBufferDescriptors(*bManager, bTextureDSetComponent->materialBuffer,
	                                         bTextureDSetComponent->bindlessTextureSet, 2);

	// Shadow Map Texture Set (binding 1)
	dManager->updateSingleTextureDSet(bTextureDSetComponent->bindlessTextureSet, 1,
	                                  tManager->getTexture(directLight->textureShadowImage).textureImageView,
	                                  tManager->getTexture(directLight->textureShadowImage).textureSampler);

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
	Texture& whiteCubemap = tManager->getTexture(whiteCubemapHandle);
	tManager->createImageView(whiteCubemap, vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor,
	                          vk::ImageViewType::eCube);
	tManager->createSampler(whiteCubemap, samplerPresets::cubemap());

	// Transition cubemap to shader-read layout so it's valid for sampling
	{
		auto cmd = VulkanUtils::beginSingleTimeCommands(*vulkanDevice);
		VulkanUtils::transitionImageLayout(
		    cmd, whiteCubemap.textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal, {},
		    vk::AccessFlagBits2::eShaderRead, vk::PipelineStageFlagBits2::eTopOfPipe,
		    vk::PipelineStageFlagBits2::eFragmentShader, vk::ImageAspectFlagBits::eColor, 6, 1);
		VulkanUtils::endSingleTimeCommands(cmd, *vulkanDevice);
	}

	dManager->update(bTextureDSetComponent->bindlessTextureSet, Bindings::Textures::CubemapSampler, 0,
	                 vk::DescriptorType::eCombinedImageSampler, whiteCubemap.textureImageView,
	                 whiteCubemap.textureSampler);
	dManager->update(bTextureDSetComponent->bindlessTextureSet, Bindings::Textures::CubemapStorage, 0,
	                 vk::DescriptorType::eStorageImage, whiteCubemap.textureImageView, nullptr,
	                 vk::ImageLayout::eGeneral);

	dManager->update(bTextureDSetComponent->bindlessTextureSet, Bindings::Textures::PrefilteredMap, 0,
	                 vk::DescriptorType::eCombinedImageSampler, whiteCubemap.textureImageView,
	                 whiteCubemap.textureSampler);
	dManager->update(bTextureDSetComponent->bindlessTextureSet, Bindings::Textures::BrdfLut, 0,
	                 vk::DescriptorType::eCombinedImageSampler, whiteCubemap.textureImageView,
	                 whiteCubemap.textureSampler);

	// BRDF LUT - generated once, reused across all skybox changes
	TextureHandle brdfLutHandle =
	    EnvMapFactory::brdfLut(*tManager, *vulkanDevice, *dManager, *bTextureDSetComponent, pManager);

	skybox->cubemapTexture = whiteCubemapHandle;
	skybox->prefilteredMap = whiteCubemapHandle;
	skybox->brdfLut = brdfLutHandle;

#pragma endregion

#pragma region Model & Frustum Culling Buffers (Set 1)
	ModelDSetComponent* objectDSetComponent = gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();
	objectDSetComponent->modelBufferDSet = dManager->allocate("modelSet", MAX_FRAMES_IN_FLIGHT);
	objectDSetComponent->bakeModelDSet = dManager->allocate("modelSet", 1);

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

#pragma region GTAO Settings
	Orhescyon::Entity gtaoSettingsEntity = gm.createEntity();
	gm.addComponent<NameComponent>(gtaoSettingsEntity, "GTAO Settings");
	gm.registerContext<GtaoSettingsContext>(gtaoSettingsEntity);
	gm.addComponent<GtaoSettingsComponent>(gtaoSettingsEntity);
#pragma endregion

#pragma region Light Probe Grid
	Orhescyon::Entity probeGridEntity = gm.createEntity();
	gm.registerContext<LightProbeGridContext>(probeGridEntity);
	gm.addComponent<LightProbeGridComponent>(
	    probeGridEntity, LightProbeGridComponent{.origin = glm::vec3(0.0f), .count = glm::ivec3(0), .spacing = 0.0f});
	gm.addComponent<NameComponent>(probeGridEntity, "SYSTEM Light Probe Grid");
#pragma endregion

	Orhescyon::Entity deltaTimeEntity = gm.createEntity();
	gm.registerContext<DeltaTimeContext>(deltaTimeEntity);
	gm.addComponent<DeltaTimeComponent>(deltaTimeEntity);
	gm.addComponent<NameComponent>(deltaTimeEntity, "SYSTEM::GRAPHICS Delta Time");

#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::PLACEHOLDERENTITYS::Succes!" << std::endl;
#endif //_DEBUG
}

void PlaceholdersInit::initAfterCorePlaceholders(GeneralManager& gm) 
{
	gm.subscribeEntity<TransformSystem>(gm.getContext<MainCameraContext>());
	gm.subscribeEntity<TransformSystem>(gm.getContext<SunContext>());
}
