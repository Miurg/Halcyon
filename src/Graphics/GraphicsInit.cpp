
#include "GraphicsInit.hpp"
#include <iostream>
#include <stdexcept>
#include "Factories/VulkanDeviceFactory.hpp"
#include "Factories/SwapChainFactory.hpp"
#include "Factories/PipelineFactory.hpp"

// Entities and Components
#include "Components/VulkanDeviceComponent.hpp"
#include "Components/SwapChainComponent.hpp"
#include "Components/VMAllocatorComponent.hpp"
#include "Components/TextureManagerComponent.hpp"
#include "Components/BufferManagerComponent.hpp"
#include "Components/ModelManagerComponent.hpp"
#include "Components/DescriptorManagerComponent.hpp"
#include "Components/FrameManagerComponent.hpp"
#include "Components/FrameDataComponent.hpp"
#include "Components/CurrentFrameComponent.hpp"
#include "Components/FrameImageComponent.hpp"
#include "Components/PipelineHandlerComponent.hpp"
#include "Resources/Components/GlobalDSetComponent.hpp"
#include "Resources/Components/ModelDSetComponent.hpp"
#include "Resources/Components/FrustumDSetComponent.hpp"
#include "Components/CameraComponent.hpp"
#include "Components/LightComponent.hpp"
#include "Components/LocalTransformComponent.hpp"
#include "Components/GlobalTransformComponent.hpp"
#include "Components/RelationshipComponent.hpp"
#include "../Platform/PlatformContexts.hpp"
#include "../Platform/Components/WindowComponent.hpp"


// Managers and Resources
#include "VulkanDevice.hpp"
#include "SwapChain.hpp"
#include "PipelineHandler.hpp"
#include "Resources/Managers/TextureManager.hpp"
#include "Resources/Managers/BufferManager.hpp"
#include "Resources/Managers/ModelManager.hpp"
#include "Resources/Managers/DescriptorManager.hpp"
#include "Managers/FrameManager.hpp"
// Contexts
#include "GraphicsContexts.hpp"

void GraphicsInit::Run(GeneralManager& gm)
{
#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::RUN::Start init" << std::endl;
#endif //_DEBUG

	initVulkanCore(gm);
	initManagers(gm);
	initFrameData(gm);
	initPipelines(gm);
	initScene(gm);

#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::RUN::Succes!" << std::endl;
#endif //_DEBUG
}

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

	// Swap Chain
	Entity swapChainEntity = gm.createEntity();
	gm.registerContext<MainSwapChainContext>(swapChainEntity);
	SwapChain* swapChain = new SwapChain();
	SwapChainFactory::createSwapChain(*swapChain, *vulkanDevice, *window);
	gm.addComponent<SwapChainComponent>(swapChainEntity, swapChain);

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
}

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

	// Buffer Manager
	Entity bufferManagerEntity = gm.createEntity();
	gm.registerContext<BufferManagerContext>(bufferManagerEntity);
	BufferManager* bManager = new BufferManager(*vulkanDevice, allocator);
	gm.addComponent<BufferManagerComponent>(bufferManagerEntity, bManager);

	// Model Manager
	Entity modelManagerEntity = gm.createEntity();
	gm.registerContext<ModelManagerContext>(modelManagerEntity);
	ModelManager* mManager = new ModelManager(*vulkanDevice, allocator);
	gm.addComponent<ModelManagerComponent>(modelManagerEntity, mManager);

	// Descriptor Manager
	Entity descriptorManagerEntity = gm.createEntity();
	gm.registerContext<DescriptorManagerContext>(descriptorManagerEntity);
	DescriptorManager* dManager = new DescriptorManager(*vulkanDevice);
	gm.addComponent<DescriptorManagerComponent>(descriptorManagerEntity, dManager);

	// Frame Manager
	Entity frameManagerEntity = gm.createEntity();
	gm.registerContext<FrameManagerContext>(frameManagerEntity);
	FrameManager* fManager = new FrameManager(*vulkanDevice);
	gm.addComponent<FrameManagerComponent>(frameManagerEntity, fManager);
}

void GraphicsInit::initFrameData(GeneralManager& gm)
{
	FrameManager* fManager = gm.getContextComponent<FrameManagerContext, FrameManagerComponent>()->frameManager;

	Entity frameDataEntity = gm.createEntity();
	gm.registerContext<MainFrameDataContext>(frameDataEntity);
	gm.registerContext<CurrentFrameContext>(frameDataEntity);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		fManager->initFrameData();
	}
	gm.addComponent<FrameDataComponent>(frameDataEntity, 0);
	gm.addComponent<CurrentFrameComponent>(frameDataEntity);

	// Frame Images
	Entity frameImageEntity = gm.createEntity();
	gm.registerContext<FrameImageContext>(frameImageEntity);
	gm.addComponent<FrameImageComponent>(frameImageEntity);
}

void GraphicsInit::initPipelines(GeneralManager& gm)
{
	VulkanDevice* vulkanDevice = gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	SwapChain* swapChain = gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	DescriptorManager* dManager = gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	
	// Main Descriptor Sets
	Entity mainDSetsEntity = gm.createEntity();
	gm.registerContext<MainDSetsContext>(mainDSetsEntity);
	gm.addComponent<BindlessTextureDSetComponent>(mainDSetsEntity);
	gm.addComponent<GlobalDSetComponent>(mainDSetsEntity);
	gm.addComponent<ModelDSetComponent>(mainDSetsEntity);
	gm.addComponent<FrustumDSetComponent>(mainDSetsEntity);

	// Signature and Pipelines
	Entity signatureEntity = gm.createEntity();
	gm.registerContext<MainSignatureContext>(signatureEntity);
	PipelineHandler* pipelineHandler = new PipelineHandler();
	PipelineFactory::createGraphicsPipeline(*vulkanDevice, *swapChain, *dManager, *pipelineHandler);
	PipelineFactory::createShadowPipeline(*vulkanDevice, *swapChain, *dManager, *pipelineHandler);
	PipelineFactory::createFxaaPipeline(*vulkanDevice, *swapChain, *dManager, *pipelineHandler);
	PipelineFactory::createCullingPipeline(*vulkanDevice, *dManager, *pipelineHandler);
	gm.addComponent<PipelineHandlerComponent>(signatureEntity, pipelineHandler);

#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::VULKANNEEDS::Succes!" << std::endl;
#endif //_DEBUG
}

void GraphicsInit::initScene(GeneralManager& gm)
{
#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::PLACEHOLDERENTITYS::Start creating placeholder entities" << std::endl;
#endif //_DEBUG
	// === Placeholder Entities ===
	DescriptorManager* dManager =
		gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	BufferManager* bManager = gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	TextureManager* tManager =
		gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	BindlessTextureDSetComponent* bTextureDSetComponent =
		gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
	GlobalDSetComponent* globalDSetComponent = gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	SwapChain* swap = gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;

	globalDSetComponent->globalDSets =
		dManager->allocateStorageBufferDSets(MAX_FRAMES_IN_FLIGHT, *dManager->globalSetLayout);

	// === Camera ===
	Entity cameraEntity = gm.createEntity();
	gm.addComponent<CameraComponent>(cameraEntity);
	gm.addComponent<GlobalTransformComponent>(cameraEntity, glm::vec3(-5.0f, 5.0f, 3.0f));
	gm.registerContext<MainCameraContext>(cameraEntity);
	CameraComponent* camera = gm.getContextComponent<MainCameraContext, CameraComponent>();
	globalDSetComponent->cameraBuffers = bManager->createBuffer(
		(vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal), sizeof(CameraStucture),
		MAX_FRAMES_IN_FLIGHT, 0, *dManager->globalSetLayout, vk::BufferUsageFlagBits::eStorageBuffer);
	dManager->updateStorageBufferDescriptors(*bManager, globalDSetComponent->cameraBuffers,
												globalDSetComponent->globalDSets, 0);
	// === Camera END ===

	// === Sun ===
	Entity sunEntity = gm.createEntity();
	gm.addComponent<CameraComponent>(sunEntity);
	gm.addComponent<GlobalTransformComponent>(sunEntity, glm::vec3(10.0f, 20.0f, 10.0f));
	gm.addComponent<LightComponent>(sunEntity, 2048, 2048, glm::vec4(1.0f, 1.0f, 1.0f, 3.0f),
									glm::vec4(1.0f, 1.0f, 1.0f, 0.1f));
	gm.registerContext<SunContext>(sunEntity);
	CameraComponent* sunCamera = gm.getContextComponent<SunContext, CameraComponent>();
	LightComponent* sunLight = gm.getContextComponent<SunContext, LightComponent>();
	globalDSetComponent->sunCameraBuffers = bManager->createBuffer(
		(vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal), sizeof(SunStructure),
		MAX_FRAMES_IN_FLIGHT, 2, *dManager->globalSetLayout, vk::BufferUsageFlagBits::eStorageBuffer);
	dManager->updateStorageBufferDescriptors(*bManager, globalDSetComponent->sunCameraBuffers,
												globalDSetComponent->globalDSets, 2);
	sunLight->textureShadowImage = tManager->createShadowMap(sunLight->sizeX, sunLight->sizeY);
	dManager->updateSingleTextureDSet(globalDSetComponent->globalDSets, 1,
										tManager->textures[sunLight->textureShadowImage].textureImageView,
										tManager->textures[sunLight->textureShadowImage].textureSampler);
	// === Sun END ===

	// === Frustum ===
	FrustumDSetComponent* frustumDSetComponent = gm.getContextComponent<MainDSetsContext, FrustumDSetComponent>();
	frustumDSetComponent->frustumBufferDSet =
		dManager->allocateStorageBufferDSets(MAX_FRAMES_IN_FLIGHT, *dManager->frustrumSetLayout);

	frustumDSetComponent->indirectDrawBuffer =
		bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal),
		sizeof(IndirectDrawStructure) * 10240, MAX_FRAMES_IN_FLIGHT, 0, *dManager->frustrumSetLayout,
								vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer |
									vk::BufferUsageFlagBits::eTransferDst);
	dManager->updateStorageBufferDescriptors(*bManager, frustumDSetComponent->indirectDrawBuffer,
												frustumDSetComponent->frustumBufferDSet, 0);
	frustumDSetComponent->visibleIndicesBuffer =
		bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal),
								sizeof(uint32_t) * 10240, MAX_FRAMES_IN_FLIGHT, 1, *dManager->frustrumSetLayout,
								vk::BufferUsageFlagBits::eStorageBuffer);
	dManager->updateStorageBufferDescriptors(*bManager, frustumDSetComponent->visibleIndicesBuffer,
												frustumDSetComponent->frustumBufferDSet, 1);
	// === Frustum END ===

	// === Bindless Texture Set ===
	bTextureDSetComponent->bindlessTextureSet = dManager->allocateBindlessTextureDSet();
	// === Bindless Texture Set END ===

	// === FXAA Descriptor Set ===
	globalDSetComponent->fxaaDSets = dManager->allocateFxaaDescriptorSet(*dManager->fxaaSetLayout);
	// === FXAA Descriptor Set END ===

	// === Offscreen Descriptor Set ===
	dManager->updateSingleTextureDSet(globalDSetComponent->fxaaDSets, 0, swap->offscreenImageView,
										swap->offscreenSampler);
	// === Offscreen Descriptor Set END ===

	// === Model ===
	ModelDSetComponent* objectDSetComponent = gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();
	objectDSetComponent->modelBufferDSet =
		dManager->allocateStorageBufferDSets(MAX_FRAMES_IN_FLIGHT, *dManager->modelSetLayout);
	objectDSetComponent->primitiveBuffer =
		bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible), 10240 * sizeof(PrimitiveSctructure), MAX_FRAMES_IN_FLIGHT, 0,
		*dManager->modelSetLayout, vk::BufferUsageFlagBits::eStorageBuffer);
	dManager->updateStorageBufferDescriptors(*bManager, objectDSetComponent->primitiveBuffer,
												objectDSetComponent->modelBufferDSet, 0);
	objectDSetComponent->transformBuffer =
		bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible), 10240 * sizeof(TransformStructure), MAX_FRAMES_IN_FLIGHT, 1,
		*dManager->modelSetLayout, vk::BufferUsageFlagBits::eStorageBuffer);
	dManager->updateStorageBufferDescriptors(*bManager, objectDSetComponent->transformBuffer,
												objectDSetComponent->modelBufferDSet, 1);
	// === ModelEND ===

	// === Placeholder Entities END ===

	// === Default White Texture ===
	auto texturePtr = GltfLoader::createDefaultWhiteTexture();
	int texWidth = texturePtr.get()->width;
	int texHeight = texturePtr.get()->height;
	auto data = texturePtr->pixels.data();
	auto path = texturePtr.get()->name.c_str();
	tManager->generateTextureData(path, texWidth, texHeight, data, *bTextureDSetComponent, *dManager);
	// === Default White Texture END ===

#ifdef _DEBUG
	std::cout << "GRAPHICSINIT::PLACEHOLDERENTITYS::Succes!" << std::endl;
#endif //_DEBUG
}
