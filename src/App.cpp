#include "App.hpp"

#include <iostream>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLFW/glfw3.h>
#include "CoreInit.hpp"
#include "Graphics/Factories/VulkanDeviceFactory.hpp"
#include "Graphics/Factories/SwapChainFactory.hpp"
#include "Graphics/Factories/DescriptorHandlerFactory.hpp"
#include "Graphics/Factories/PipelineFactory.hpp"

#include "Platform/Components/WindowComponent.hpp"
#include "Platform/Components/KeyboardStateComponent.hpp"
#include "Platform/Components/MouseStateComponent.hpp"
#include "Platform/Components/CursorPositionComponent.hpp"
#include "Platform/Components/WindowSizeComponent.hpp"
#include "Platform/Components/ScrollDeltaComponent.hpp"
#include "Platform/Systems/InputSolverSystem.hpp"
#include "Platform/PlatformContexts.hpp"
#include "Graphics/Components/VulkanDeviceComponent.hpp"
#include "Graphics/GraphicsContexts.hpp"
#include "Graphics/Components/SwapChainComponent.hpp"
#include "Graphics/Components/AssetManagerComponent.hpp"
#include "Graphics/Components/DescriptorHandlerComponent.hpp"
#include "Graphics/Components/PipelineHandlerComponent.hpp"
#include "Graphics/Components/FrameDataComponent.hpp"
#include "Graphics/Components/CurrentFrameComponent.hpp"
#include "Graphics/Components/GameObjectComponent.hpp"
#include "Graphics/Systems/RenderSystem.hpp"
#include "Graphics/Components/TransformComponent.hpp"


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
	
	Entity swapChainEntity = gm.createEntity();
	gm.registerContext<MainSwapChainContext>(swapChainEntity);
	swapChain = new SwapChain();
	SwapChainFactory::createSwapChain(*swapChain, *vulkanDevice, *window);
	gm.addComponent<SwapChainComponent>(swapChainEntity, swapChain);
	
	Entity assetManagerEntity = gm.createEntity();
	gm.registerContext<AssetManagerContext>(assetManagerEntity);
	assetManager = new AssetManager(*vulkanDevice);
	gm.addComponent<AssetManagerComponent>(assetManagerEntity, assetManager);

	Entity signatureEntity = gm.createEntity();
	gm.registerContext<MainSignatureContext>(signatureEntity);
	descriptorHandler = new DescriptorHandler();
	DescriptorHandlerFactory::createDescriptorSetLayout(*vulkanDevice, *descriptorHandler);
	DescriptorHandlerFactory::createDescriptorPool(*vulkanDevice, *descriptorHandler);
	gm.addComponent<DescriptorHandlerComponent>(signatureEntity, descriptorHandler);
	
	pipelineHandler = new PipelineHandler();
	PipelineFactory::createGraphicsPipeline(*vulkanDevice, *swapChain, *descriptorHandler, *pipelineHandler);
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

	Entity cameraEntity = gm.createEntity();
	gm.addComponent<CameraComponent>(cameraEntity, glm::vec3(0.0f, 0.0f, 3.0f));
	gm.registerContext<MainCameraContext>(cameraEntity);

	App::setupGameObjects(gm);
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
}

void App::setupGameObjects(GeneralManager& gm)
{
	for (int i = 0; i < 100; i++)
	{
		GameObject gameObject = GameObject();
		gameObjects.push_back(std::move(gameObject));
	}
	int j = 0;
	int k = 0;
	for (int i = 0; i < 10; i++)
	{
		GameObject& gameObject = gameObjects[i];
		MeshInfoComponent meshInfo;
		TextureInfoComponent textureInfo;
		if (i > 5)
		{
			meshInfo = assetManager->createMesh("assets/models/BlenderMonkey.obj");
			textureInfo = assetManager->generateTextureData("assets/textures/texture.jpg");
		}
		else
		{
			meshInfo = assetManager->createMesh("assets/models/viking_room.obj");
			textureInfo = assetManager->generateTextureData("assets/textures/viking_room.png");
		}

		GameObject::initUniformBuffers(gameObjects[i], *vulkanDevice);
		DescriptorHandlerFactory::allocateDescriptorSets(*vulkanDevice, *descriptorHandler, gameObjects[i]);
		DescriptorHandlerFactory::updateUniformDescriptors(*vulkanDevice, gameObjects[i]);
		DescriptorHandlerFactory::updateTextureDescriptors(*vulkanDevice, gameObjects[i], textureInfo, *assetManager);

		Entity gameObjectEntity1 = gm.createEntity();
		gm.addComponent<GameObjectComponent>(gameObjectEntity1, &gameObject);
		gm.addComponent<TransformComponent>(gameObjectEntity1, k * 2.0f, 0.0f, j * 2.0f);
		gm.addComponent<MeshInfoComponent>(gameObjectEntity1, meshInfo);
		gm.addComponent<TextureInfoComponent>(gameObjectEntity1, textureInfo);
		gm.subscribeEntity<RenderSystem>(gameObjectEntity1);


		k++;
		if ((i + 1) % 10 == 0)
		{
			j++;
			k = 0;
		}
	}
}