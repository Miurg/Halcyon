#include "App.hpp"

#include <iostream>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLFW/glfw3.h>
#include "CoreInit.hpp"
#include "Graphics/Factories/VulkanDeviceFactory.hpp"
#include "Graphics/Factories/SwapChainFactory.hpp"
#include "Graphics/Factories/PipelineFactory.hpp"
#include "Platform/Components/WindowComponent.hpp"
#include "Platform/PlatformContexts.hpp"
#include "Graphics/Components/VulkanDeviceComponent.hpp"
#include "Graphics/GraphicsContexts.hpp"
#include "Graphics/Components/SwapChainComponent.hpp"
#include "Graphics/Components/BufferManagerComponent.hpp"
#include "Graphics/Components/PipelineHandlerComponent.hpp"
#include "Graphics/Components/FrameDataComponent.hpp"
#include "Graphics/Components/CurrentFrameComponent.hpp"
#include "Graphics/Components/TransformComponent.hpp"
#include "Game/Components/ControlComponent.hpp"
#include "Graphics/Components/LightComponent.hpp"
#include "Game/GameInit.hpp"
#include "Graphics/Components/DescriptorManagerComponent.hpp"
#include "Graphics/Resources/Components/GlobalDSetComponent.hpp"
#include "Graphics/Resources/Components/ObjectDSetComponent.hpp"

namespace
{
float deltaTime = 0.0f;
float lastFrame = 0.0f;
} // anonymous namespace

App::App() {}

void App::run()
{
	//=== ECS ===
	GeneralManager gm;

	CoreInit::Run(gm);

	//=== ECS END ===

	window = gm.getContextComponent<MainWindowContext, WindowComponent>()->windowInstance;

	Entity vulkanDeviceEntity = gm.createEntity();
	gm.registerContext<MainVulkanDeviceContext>(vulkanDeviceEntity);
	vulkanDevice = new VulkanDevice();
	VulkanDeviceFactory::createVulkanDevice(*window, *vulkanDevice);
	gm.addComponent<VulkanDeviceComponent>(vulkanDeviceEntity, vulkanDevice);

	Entity cameraEntity = gm.createEntity();
	gm.addComponent<CameraComponent>(cameraEntity);
	gm.addComponent<TransformComponent>(cameraEntity, glm::vec3(0.0f, 0.0f, 3.0f));
	gm.addComponent<ControlComponent>(cameraEntity);
	gm.addComponent<LightComponent>(cameraEntity, 2048, 2048);
	gm.registerContext<MainCameraContext>(cameraEntity);

	Entity sunEntity = gm.createEntity();
	gm.addComponent<CameraComponent>(sunEntity);
	gm.addComponent<TransformComponent>(sunEntity, glm::vec3(10.0f, 20.0f, 10.0f));
	gm.registerContext<LightCameraContext>(sunEntity);

	Entity swapChainEntity = gm.createEntity();
	gm.registerContext<MainSwapChainContext>(swapChainEntity);
	swapChain = new SwapChain();
	SwapChainFactory::createSwapChain(*swapChain, *vulkanDevice, *window);
	gm.addComponent<SwapChainComponent>(swapChainEntity, swapChain);

	Entity bufferManagerEntity = gm.createEntity();
	gm.registerContext<BufferManagerContext>(bufferManagerEntity);
	bManager = new BufferManager(*vulkanDevice);
	gm.addComponent<BufferManagerComponent>(bufferManagerEntity, bManager);

	Entity descriptorManagerEntity = gm.createEntity();
	gm.registerContext<DescriptorManagerContext>(descriptorManagerEntity);
	dManager = new DescriptorManager(*vulkanDevice);
	gm.addComponent<DescriptorManagerComponent>(descriptorManagerEntity, dManager);

	Entity signatureEntity = gm.createEntity();
	gm.registerContext<MainSignatureContext>(signatureEntity);
	pipelineHandler = new PipelineHandler();
	PipelineFactory::createGraphicsPipeline(*vulkanDevice, *swapChain, *dManager, *pipelineHandler);
	PipelineFactory::createShadowPipeline(*vulkanDevice, *swapChain, *dManager, *pipelineHandler);
	gm.addComponent<PipelineHandlerComponent>(signatureEntity, pipelineHandler);

	Entity frameDataEntity = gm.createEntity();
	gm.registerContext<MainFrameDataContext>(frameDataEntity);
	gm.registerContext<CurrentFrameContext>(frameDataEntity);
	std::vector<FrameData>* framesData = new std::vector<FrameData>(MAX_FRAMES_IN_FLIGHT);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		FrameData::initFrameData((*framesData)[i], *vulkanDevice);
	}
	gm.addComponent<FrameDataComponent>(frameDataEntity, framesData);
	gm.addComponent<CurrentFrameComponent>(frameDataEntity);

	CameraComponent* camera = gm.getContextComponent<MainCameraContext, CameraComponent>();
	CameraComponent* sun = gm.getContextComponent<LightCameraContext, CameraComponent>();
	LightComponent* cameraLight = gm.getContextComponent<MainCameraContext, LightComponent>();

	Entity mainDSetsEntity = gm.createEntity();
	gm.registerContext<MainDSetsContext>(mainDSetsEntity);
	gm.addComponent<BindlessTextureDSetComponent>(mainDSetsEntity);
	gm.addComponent<GlobalDSetComponent>(mainDSetsEntity);
	gm.addComponent<ObjectDSetComponent>(mainDSetsEntity);

	BindlessTextureDSetComponent* bTextureDSetComponent =
	    gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
	bTextureDSetComponent->bindlessTextureSet = dManager->allocateBindlessTextureDSet();

	GlobalDSetComponent* globalDSetComponent = gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	globalDSetComponent->cameraBuffers =
	    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal),
	                           sizeof(CameraStucture), MAX_FRAMES_IN_FLIGHT, 0, *dManager->globalSetLayout);
	camera->bufferNubmer = globalDSetComponent->cameraBuffers;
	globalDSetComponent->globalDSets = dManager->allocateStorageBufferDSets(MAX_FRAMES_IN_FLIGHT, *dManager->globalSetLayout);
	dManager->updateStorageBufferDescriptors(*bManager, globalDSetComponent->cameraBuffers,
	                                         globalDSetComponent->globalDSets, 0);

	cameraLight->textureShadowImage = bManager->createShadowMap(cameraLight->sizeX, cameraLight->sizeY);
	dManager->updateShadowDSet(globalDSetComponent->globalDSets,
	                           bManager->textures[cameraLight->textureShadowImage].textureImageView,
	                           bManager->textures[cameraLight->textureShadowImage].textureSampler);

	ObjectDSetComponent* objectDSetComponent = gm.getContextComponent<MainDSetsContext, ObjectDSetComponent>();
	objectDSetComponent->storageBuffer =
	    bManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible), 1024 * sizeof(ModelSctructure),
	                           MAX_FRAMES_IN_FLIGHT, 0, *dManager->modelSetLayout);
	objectDSetComponent->storageBufferDSet =
	    dManager->allocateStorageBufferDSets(MAX_FRAMES_IN_FLIGHT, *dManager->modelSetLayout);
	dManager->updateStorageBufferDescriptors(*bManager, objectDSetComponent->storageBuffer,
	                                         objectDSetComponent->storageBufferDSet, 0);
	GameInit::gameInitStart(gm);
	App::mainLoop(gm);
	App::cleanup();
}

void App::mainLoop(GeneralManager& gm)
{
	while (!window->shouldClose())
	{
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		glfwPollEvents();
		gm.update(deltaTime);

		frameCount++;
		float now = static_cast<float>(glfwGetTime());
		if (now - time >= 1.0f)
		{
			std::cout << "FPS: " << frameCount << std::endl;
			frameCount = 0;
			time = now;
		}
	}
}

void App::cleanup()
{
	vulkanDevice->device.waitIdle();

	SwapChainFactory::cleanupSwapChain(*swapChain);
	bManager->~BufferManager();
}
