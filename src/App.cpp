#include "App.hpp"

#include <iostream>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLFW/glfw3.h>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "CoreInit.hpp"
#include "Graphics/VulkanUtils.hpp"
#include "Graphics/Factories/MaterialAsset.hpp"
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


namespace
{
unsigned int ScreenWidth = 1920;
unsigned int ScreenHeight = 1080;

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

	Entity windowAndInputEntity = gm.createEntity();
	gm.registerContext<InputDataContext>(windowAndInputEntity);
	gm.registerContext<MainWindowContext>(windowAndInputEntity);
	window = new Window("Halcyon");
	gm.addComponent<WindowComponent>(windowAndInputEntity, window);
	gm.addComponent<KeyboardStateComponent>(windowAndInputEntity);
	gm.addComponent<MouseStateComponent>(windowAndInputEntity);
	gm.addComponent<CursorPositionComponent>(windowAndInputEntity);
	gm.addComponent<WindowSizeComponent>(windowAndInputEntity, ScreenWidth, ScreenHeight);
	gm.addComponent<ScrollDeltaComponent>(windowAndInputEntity);
	gm.subscribeEntity<InputSolverSystem>(windowAndInputEntity);

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
	const std::string TEXTURE_PATH = "assets/textures/viking_room.png";
	for (int i = 0; i < 100; i++)
	{
		GameObject gameObject = GameObject();
		gameObjects.push_back(std::move(gameObject));
	}
	int j = 0;
	int k = 0;
	for (int i = 0; i < 10; i++)
	{
		MeshInfoComponent meshInfo = assetManager->createMesh("assets/models/viking_room.obj");
		GameObject& gameObject = gameObjects[i];
		gameObject.position = {k * 2, 0.0f, j*2};
		gameObject.rotation = {0.0f, 0.0f, 0.0f};
		gameObject.scale = {1.0f, 1.0f, 1.0f};
		gameObject.meshInfo = meshInfo;
		MaterialAsset::generateTextureData(TEXTURE_PATH, *vulkanDevice, gameObject);
		Entity gameObjectEntity1 = gm.createEntity();
		gm.addComponent<GameObjectComponent>(gameObjectEntity1, &gameObject);
		gm.subscribeEntity<RenderSystem>(gameObjectEntity1);

		GameObject::initUniformBuffers(gameObjects[i], *vulkanDevice);
		DescriptorHandlerFactory::allocateDescriptorSets(*vulkanDevice, *descriptorHandler, gameObjects[i]);
		DescriptorHandlerFactory::updateUniformDescriptors(*vulkanDevice, gameObjects[i]);
		DescriptorHandlerFactory::updateTextureDescriptors(*vulkanDevice, gameObjects[i]);
		k++;
		if ((i + 1) % 10 == 0)
		{
			j++;
			k = 0;
		}
	}
}