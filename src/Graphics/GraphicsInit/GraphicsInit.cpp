
#pragma region Includes
#include "GraphicsInit.hpp"
#include <iostream>
#include <stdexcept>
#include "../Factories/VulkanDeviceFactory.hpp"
#include "../Factories/SwapChainFactory.hpp"

// Entities and Components
#include "../Components/VulkanDeviceComponent.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../Components/VMAllocatorComponent.hpp"
#include "../Components/TextureManagerComponent.hpp"
#include "../Components/BufferManagerComponent.hpp"
#include "../Components/ModelManagerComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Components/FrameManagerComponent.hpp"
#include "../Components/FrameDataComponent.hpp"
#include "../Components/CurrentFrameComponent.hpp"
#include "../Components/GraphicsSettingsComponent.hpp"
#include "../Components/FrameImageComponent.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"
#include "../Resources/Components/ModelDSetComponent.hpp"
#include "../Components/DrawInfoComponent.hpp"
#include "../Components/SsaoSettingsComponent.hpp"
#include "../Components/RenderGraphComponent.hpp"
#include "../RenderGraph/RenderGraph.hpp"

#include "../Components/NameComponent.hpp"
#include "../Components/CameraComponent.hpp"
#include "../Components/DirectLightComponent.hpp"
#include "../Components/LocalTransformComponent.hpp"
#include "../Components/GlobalTransformComponent.hpp"
#include "../Components/SkyboxComponent.hpp"
#include "../../Platform/PlatformContexts.hpp"
#include "../../Platform/Components/WindowComponent.hpp"

// Managers and Resources
#include "../VulkanDevice.hpp"
#include "../SwapChain.hpp"
#include "../Resources/Managers/TextureManager.hpp"
#include "../Resources/Managers/BufferManager.hpp"
#include "../Resources/Managers/ModelManager.hpp"
#include "../Resources/Managers/DescriptorManager.hpp"
#include "../Managers/FrameManager.hpp"
#include "../Resources/Factories/TextureUploader.hpp"
#include "../VulkanUtils.hpp"
// Contexts
#include "../GraphicsContexts.hpp"
#include "../Resources/ResourceStructures.hpp"
#include "../Resources/Managers/Bindings.hpp"
#include "GraphicsPipelinesInit.hpp"

// ImGui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include "../Components/PipelineManagerComponent.hpp"
#include "../Managers/PipelineManager.hpp"
#pragma endregion

#pragma region Run
void GraphicsInit::Run(GeneralManager& gm)
{
#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::RUN::Start init" << std::endl;
#endif //_DEBUG

	initVulkanCore(gm);
	initManagers(gm);
	initFrameData(gm);
	GraphicsPipelinesInit::initPipelines(gm);
	initScene(gm);
	initImGui(gm);

#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::RUN::Succes!" << std::endl;
#endif //_DEBUG
}
#pragma endregion

#pragma region initVulkanCore
void GraphicsInit::initVulkanCore(GeneralManager& gm)
{
#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::VULKANNEEDS::Start create vulkan needs" << std::endl;
#endif //_DEBUG
	// === Vulkan needs ===
	Window* window = gm.getContextComponent<MainWindowContext, WindowComponent>()->windowInstance;

	// Vulkan Device
	Entity vulkanDeviceEntity = gm.createEntity();
	gm.registerContext<MainVulkanDeviceContext>(vulkanDeviceEntity);
	VulkanDevice* vulkanDevice = new VulkanDevice();
	VulkanDeviceFactory::createVulkanDevice(*window, *vulkanDevice);
	gm.addComponent<VulkanDeviceComponent>(vulkanDeviceEntity, vulkanDevice);
	gm.addComponent<NameComponent>(vulkanDeviceEntity, "SYSTEM Vulkan Device");

	// VMA Allocator
	Entity vmaAllocatorEntity = gm.createEntity();
	gm.registerContext<VMAllocatorContext>(vmaAllocatorEntity);
	VmaAllocator allocator;
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = *vulkanDevice->physicalDevice;
	allocatorInfo.device = *vulkanDevice->device;
	allocatorInfo.instance = *vulkanDevice->instance;
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_4;
	VmaVulkanFunctions vulkanFunctions = {};
	vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

	allocatorInfo.pVulkanFunctions = &vulkanFunctions;
	VkResult result = vmaCreateAllocator(&allocatorInfo, &allocator);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create VMA allocator!");
	}
	gm.addComponent<VMAllocatorComponent>(vmaAllocatorEntity, allocator);
	gm.addComponent<NameComponent>(vmaAllocatorEntity, "SYSTEM VMA Allocator");
}
#pragma endregion

#pragma region initManagers
void GraphicsInit::initManagers(GeneralManager& gm)
{
	VulkanDevice* vulkanDevice =
	    gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	VmaAllocator allocator = gm.getContextComponent<VMAllocatorContext, VMAllocatorComponent>()->allocator;

	// Texture Manager
	Entity textureManagerEntity = gm.createEntity();
	gm.registerContext<TextureManagerContext>(textureManagerEntity);
	TextureManager* textureManager = new TextureManager(*vulkanDevice, allocator);
	gm.addComponent<TextureManagerComponent>(textureManagerEntity, textureManager);
	gm.addComponent<NameComponent>(textureManagerEntity, "SYSTEM Texture Manager");

	// Buffer Manager
	Entity bufferManagerEntity = gm.createEntity();
	gm.registerContext<BufferManagerContext>(bufferManagerEntity);
	BufferManager* bManager = new BufferManager(*vulkanDevice, allocator);
	gm.addComponent<BufferManagerComponent>(bufferManagerEntity, bManager);
	gm.addComponent<NameComponent>(bufferManagerEntity, "SYSTEM Buffer Manager");

	// Model Manager
	Entity modelManagerEntity = gm.createEntity();
	gm.registerContext<ModelManagerContext>(modelManagerEntity);
	ModelManager* mManager = new ModelManager(*vulkanDevice, allocator);
	gm.addComponent<ModelManagerComponent>(modelManagerEntity, mManager);
	gm.addComponent<NameComponent>(modelManagerEntity, "SYSTEM Model Manager");

	// Descriptor Manager
	Entity descriptorManagerEntity = gm.createEntity();
	gm.registerContext<DescriptorManagerContext>(descriptorManagerEntity);
	DescriptorManager* dManager = new DescriptorManager(*vulkanDevice);
	gm.addComponent<DescriptorManagerComponent>(descriptorManagerEntity, dManager);
	gm.addComponent<NameComponent>(descriptorManagerEntity, "SYSTEM Descriptor Manager");

	// Frame Manager
	Entity frameManagerEntity = gm.createEntity();
	gm.registerContext<FrameManagerContext>(frameManagerEntity);
	FrameManager* fManager = new FrameManager(*vulkanDevice);
	gm.addComponent<FrameManagerComponent>(frameManagerEntity, fManager);
	gm.addComponent<NameComponent>(frameManagerEntity, "SYSTEM Frame Manager");
}
#pragma endregion

#pragma region initFrameData
void GraphicsInit::initFrameData(GeneralManager& gm)
{
	FrameManager* fManager = gm.getContextComponent<FrameManagerContext, FrameManagerComponent>()->frameManager;
	VulkanDevice* vulkanDevice =
	    gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	Window* window = gm.getContextComponent<MainWindowContext, WindowComponent>()->windowInstance;
	TextureManager* tManager = gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;

	Entity frameDataEntity = gm.createEntity();
	gm.registerContext<MainFrameDataContext>(frameDataEntity);
	gm.registerContext<CurrentFrameContext>(frameDataEntity);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		fManager->initFrameData();
	}
	gm.addComponent<FrameDataComponent>(frameDataEntity, 0);
	gm.addComponent<CurrentFrameComponent>(frameDataEntity);
	gm.addComponent<DrawInfoComponent>(frameDataEntity);
	gm.addComponent<NameComponent>(frameDataEntity, "SYSTEM Frame Data");

	// Frame Images
	Entity frameImageEntity = gm.createEntity();
	gm.registerContext<FrameImageContext>(frameImageEntity);
	gm.addComponent<FrameImageComponent>(frameImageEntity);
	gm.addComponent<NameComponent>(frameImageEntity, "SYSTEM Frame Image");

	// Swap Chain
	Entity swapChainEntity = gm.createEntity();
	gm.registerContext<MainSwapChainContext>(swapChainEntity);
	SwapChain* swapChain = new SwapChain();
	SwapChainFactory::createSwapChain(*swapChain, *vulkanDevice, *window);
	gm.addComponent<SwapChainComponent>(swapChainEntity, swapChain);
	gm.addComponent<NameComponent>(swapChainEntity, "SYSTEM Swap Chain");

	// Main Descriptor Sets
	Entity mainDSetsEntity = gm.createEntity();
	gm.registerContext<MainDSetsContext>(mainDSetsEntity);
	gm.addComponent<BindlessTextureDSetComponent>(mainDSetsEntity);
	gm.addComponent<GlobalDSetComponent>(mainDSetsEntity);
	gm.addComponent<ModelDSetComponent>(mainDSetsEntity);
	gm.addComponent<NameComponent>(mainDSetsEntity, "SYSTEM Descriptor Sets");
}
#pragma endregion

#pragma region initScene
void GraphicsInit::initScene(GeneralManager& gm)
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
	Entity cameraEntity = gm.createEntity();
	gm.addComponent<NameComponent>(cameraEntity, "Main Camera");
	gm.addComponent<CameraComponent>(cameraEntity);
	gm.addComponent<GlobalTransformComponent>(cameraEntity, glm::vec3(-5.0f, 5.0f, 3.0f));
	gm.registerContext<MainCameraContext>(cameraEntity);
	CameraComponent* camera = gm.getContextComponent<MainCameraContext, CameraComponent>();

	// === DirectLight ===
	Entity directLightEntity = gm.createEntity();
	gm.addComponent<NameComponent>(directLightEntity, "Directional Light (Sun)");
	gm.addComponent<CameraComponent>(directLightEntity);
	glm::vec3 directLightPos = glm::vec3(10.0f, 20.0f, 10.0f);
	glm::vec3 directLightDir = glm::normalize(-directLightPos);
	glm::quat directLightRot = glm::quatLookAt(directLightDir, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec4 directLightColor = glm::vec4(0.984f, 1.0f, 0.808f, 50.0f); // Slightly warm white light with high intensity
	glm::vec4 directLightAmbient = directLightColor * 0.01f;                     // Ambient component is 10% of the main light color
	gm.addComponent<GlobalTransformComponent>(directLightEntity, directLightPos, directLightRot);
	gm.addComponent<DirectLightComponent>(directLightEntity, 4000, 4000, directLightColor, directLightAmbient);
	gm.registerContext<SunContext>(directLightEntity);
	CameraComponent* directLightCamera = gm.getContextComponent<SunContext, CameraComponent>();
	DirectLightComponent* directLight = gm.getContextComponent<SunContext, DirectLightComponent>();
	directLight->textureShadowImage = tManager->createShadowMap(directLight->sizeX, directLight->sizeY);

#pragma endregion
	// === Graphics Settings ===
	Entity settingsEntity = gm.createEntity();
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
	    dManager->allocateStorageBufferDSets(MAX_FRAMES_IN_FLIGHT, *dManager->globalSetLayout);

	// Camera buffer
	globalDSetComponent->cameraBuffers =
	    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal),
	                           sizeof(CameraStructure), MAX_FRAMES_IN_FLIGHT, vk::BufferUsageFlagBits::eStorageBuffer);
	dManager->updateStorageBufferDescriptors(*bManager, globalDSetComponent->cameraBuffers,
	                                         globalDSetComponent->globalDSets, Bindings::Global::Camera);

	// Sun buffer
	globalDSetComponent->sunCameraBuffers =
	    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal),
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
	Entity skyboxEntity = gm.createEntity();
	gm.addComponent<NameComponent>(skyboxEntity, "Skybox");
	gm.addComponent<SkyboxComponent>(skyboxEntity);
	gm.registerContext<SkyBoxContext>(skyboxEntity);
	SkyboxComponent* skybox = gm.getContextComponent<SkyBoxContext, SkyboxComponent>();

	TextureHandle whiteCubemapHandle = tManager->createCubemapImage(1, 1, vk::Format::eR32G32B32A32Sfloat);
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

	// Default zeroed SH buffer (9 x float4 = 144 bytes) - used until a real skybox is loaded
	constexpr vk::DeviceSize shBufferSize = 9 * 3 * sizeof(float);
	BufferHandle defaultSHHandle = bManager->createBuffer(
	    vk::MemoryPropertyFlagBits::eDeviceLocal, shBufferSize, 1,
	    vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
	{
		auto cmd = VulkanUtils::beginSingleTimeCommands(*vulkanDevice);
		cmd.fillBuffer(bManager->buffers[defaultSHHandle.id].buffer[0], 0, VK_WHOLE_SIZE, 0);
		VulkanUtils::endSingleTimeCommands(cmd, *vulkanDevice);
	}
	dManager->updateSHBufferDescriptor(*bTextureDSetComponent, bManager->buffers[defaultSHHandle.id].buffer[0],
	                                    shBufferSize);

	dManager->updateIBLDescriptors(*bTextureDSetComponent, whiteCubemap.textureImageView, whiteCubemap.textureSampler,
	                               whiteCubemap.textureImageView, whiteCubemap.textureSampler);

	// BRDF LUT - generated once, reused across all skybox changes
	TextureHandle brdfLutHandle = tManager->generateBrdfLut(*dManager, *bTextureDSetComponent, pManager);

	skybox->cubemapTexture = whiteCubemapHandle;
	skybox->shBuffer = defaultSHHandle;
	skybox->prefilteredMap = whiteCubemapHandle;
	skybox->brdfLut = brdfLutHandle;

#pragma endregion

#pragma region Model & Frustum Culling Buffers (Set 1)
	ModelDSetComponent* objectDSetComponent = gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();
	objectDSetComponent->modelBufferDSet =
	    dManager->allocateStorageBufferDSets(MAX_FRAMES_IN_FLIGHT, *dManager->modelSetLayout);

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
	globalDSetComponent->fxaaDSets = dManager->allocateOffscreenDescriptorSet(*dManager->screenSpaceSetLayout);
	globalDSetComponent->ssaoDSets = dManager->allocateOffscreenDescriptorSet(*dManager->screenSpaceSetLayout);
	globalDSetComponent->ssaoBlurHDSets = dManager->allocateOffscreenDescriptorSet(*dManager->screenSpaceSetLayout);
	globalDSetComponent->ssaoBlurVDSets = dManager->allocateOffscreenDescriptorSet(*dManager->screenSpaceSetLayout);
	globalDSetComponent->ssaoApplyDSets = dManager->allocateOffscreenDescriptorSet(*dManager->screenSpaceSetLayout);
	globalDSetComponent->toneMappingDSets = dManager->allocateOffscreenDescriptorSet(*dManager->screenSpaceSetLayout);
	globalDSetComponent->vignetteDSets = dManager->allocateOffscreenDescriptorSet(*dManager->screenSpaceSetLayout);

	// Bloom descriptor sets (5 downsample + 5 upsample)
	for (int i = 0; i < 5; i++)
	{
		globalDSetComponent->bloomDownsampleDSets[i] = dManager->allocateOffscreenDescriptorSet(*dManager->screenSpaceSetLayout);
		globalDSetComponent->bloomUpsampleDSets[i] = dManager->allocateOffscreenDescriptorSet(*dManager->screenSpaceSetLayout);
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
	                                       tManager->textures[swap->ssaoNoiseTextureHandle.id], allocator, *vulkanDevice);
	dManager->updateSingleTextureDSet(globalDSetComponent->ssaoDSets, Bindings::SSAO::NoiseInput,
	                                  tManager->textures[swap->ssaoNoiseTextureHandle.id].textureImageView,
	                                  tManager->textures[swap->ssaoNoiseTextureHandle.id].textureSampler);
#pragma endregion

#pragma region SSAO Settings
	Entity ssaoSettingsEntity = gm.createEntity();
	gm.addComponent<NameComponent>(ssaoSettingsEntity, "SSAO Settings");
	gm.registerContext<SsaoSettingsContext>(ssaoSettingsEntity);
	gm.addComponent<SsaoSettingsComponent>(ssaoSettingsEntity);
#pragma endregion

#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::PLACEHOLDERENTITYS::Succes!" << std::endl;
#endif //_DEBUG
}
#pragma endregion // initScene

#pragma region initImGui
void GraphicsInit::initImGui(GeneralManager& gm)
{
#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::IMGUI::Start init ImGui" << std::endl;
#endif //_DEBUG

	VulkanDevice* vulkanDevice =
	    gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	Window* window = gm.getContextComponent<MainWindowContext, WindowComponent>()->windowInstance;
	SwapChain* swapChain = gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	DescriptorManager* dManager =
	    gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForVulkan(window->getHandle(), true);

	ImGui_ImplVulkan_InitInfo initInfo = {};
	initInfo.Instance = *vulkanDevice->instance;
	initInfo.PhysicalDevice = *vulkanDevice->physicalDevice;
	initInfo.Device = *vulkanDevice->device;
	initInfo.QueueFamily = vulkanDevice->graphicsIndex;
	initInfo.Queue = *vulkanDevice->graphicsQueue;
	initInfo.PipelineCache = nullptr;
	initInfo.DescriptorPool = *dManager->imguiPool;
	initInfo.RenderPass = nullptr; // For dynamic rendering
	initInfo.Subpass = 0;
	initInfo.MinImageCount = 2; // Usually 2 or 3
	initInfo.ImageCount = MAX_FRAMES_IN_FLIGHT;
	initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	initInfo.Allocator = nullptr;
	initInfo.CheckVkResultFn = nullptr;

	// Dynamic rendering specific setup
	VkPipelineRenderingCreateInfoKHR pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
	VkFormat colorAttachmentFormat = (VkFormat)swapChain->swapChainImageFormat;
	pipelineInfo.colorAttachmentCount = 1;
	pipelineInfo.pColorAttachmentFormats = &colorAttachmentFormat;

	initInfo.UseDynamicRendering = true;
	initInfo.PipelineRenderingCreateInfo = pipelineInfo;

	ImGui_ImplVulkan_Init(&initInfo);

	ImGui_ImplVulkan_CreateFontsTexture();

#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::IMGUI::Succes!" << std::endl;
#endif //_DEBUG
}
#pragma endregion