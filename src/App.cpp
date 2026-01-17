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
#include "Graphics/Resources/Components/ModelsBuffersComponent.hpp"
#include "Graphics/Components/DescriptorManagerComponent.hpp"

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

	CameraComponent* camera = gm.getContextComponent<MainCameraContext, CameraComponent>();
	CameraComponent* sun = gm.getContextComponent<LightCameraContext, CameraComponent>();
	LightComponent* cameraLight = gm.getContextComponent<MainCameraContext, LightComponent>();

	Entity swapChainEntity = gm.createEntity();
	gm.registerContext<MainSwapChainContext>(swapChainEntity);
	swapChain = new SwapChain();
	SwapChainFactory::createSwapChain(*swapChain, *vulkanDevice, *window);
	gm.addComponent<SwapChainComponent>(swapChainEntity, swapChain);

	Entity mainDSetsEntity = gm.createEntity();
	gm.registerContext<MainDSetsContext>(mainDSetsEntity);
	gm.addComponent<BindlessTextureDSetComponent>(mainDSetsEntity);

	Entity bufferManagerEntity = gm.createEntity();
	gm.registerContext<BufferManagerContext>(bufferManagerEntity);
	bufferManager = new BufferManager(*vulkanDevice);
	gm.addComponent<BufferManagerComponent>(bufferManagerEntity, bufferManager);

	Entity descriptorManagerEntity = gm.createEntity();
	gm.registerContext<DescriptorManagerContext>(descriptorManagerEntity);
	descriptorManager = new DescriptorManager(*vulkanDevice);
	gm.addComponent<DescriptorManagerComponent>(descriptorManagerEntity, descriptorManager);

	Entity signatureEntity = gm.createEntity();
	gm.registerContext<MainSignatureContext>(signatureEntity);
	pipelineHandler = new PipelineHandler();
	PipelineFactory::createGraphicsPipeline(*vulkanDevice, *swapChain, *descriptorManager, *pipelineHandler);
	PipelineFactory::createShadowPipeline(*vulkanDevice, *swapChain, *descriptorManager, *pipelineHandler);
	gm.addComponent<PipelineHandlerComponent>(signatureEntity, pipelineHandler);

	descriptorManager->allocateBindlessTextureDSet(*gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>());

	camera->descriptorNumber = bufferManager->createBuffer(
	    (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal), sizeof(CameraStucture),
	    MAX_FRAMES_IN_FLIGHT, 0, *descriptorManager->globalSetLayout, *descriptorManager);

	cameraLight->textureShadowImage = bufferManager->createShadowMap(cameraLight->sizeX, cameraLight->sizeY);
	descriptorManager->updateShadowDSet(camera->descriptorNumber,
	                             bufferManager->textures[cameraLight->textureShadowImage].textureImageView,
	                             bufferManager->textures[cameraLight->textureShadowImage].textureSampler, *bufferManager);

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

	Entity modelSSBOsEntity = gm.createEntity();
	gm.registerContext<ModelSSBOsContext>(modelSSBOsEntity);
	int descriptorNumber =
	    bufferManager->createBuffer((vk::MemoryPropertyFlagBits::eHostVisible), 1024 * sizeof(ModelSctructure),
	                                MAX_FRAMES_IN_FLIGHT, 0, *descriptorManager->modelSetLayout, *descriptorManager);
	gm.addComponent<ModelsBuffersComponent>(modelSSBOsEntity, descriptorNumber);

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
	bufferManager->~BufferManager();
}
