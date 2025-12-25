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
#include "Graphics/Components/ModelHandlerComponent.hpp"
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
	
	Entity modelEntity = gm.createEntity();
	gm.registerContext<MainModelContext>(modelEntity);
	model = new Model(*vulkanDevice);
	gm.addComponent<ModelHandlerComponent>(modelEntity, model);

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

	Entity gameObjectEntity1 = gm.createEntity();
	Entity gameObjectEntity2 = gm.createEntity();
	Entity gameObjectEntity3 = gm.createEntity();
	gm.addComponent<GameObjectComponent>(gameObjectEntity1, &gameObjects[0]);
	gm.addComponent<GameObjectComponent>(gameObjectEntity2, &gameObjects[1]);
	gm.addComponent<GameObjectComponent>(gameObjectEntity3, &gameObjects[2]);
	gm.subscribeEntity<RenderSystem>(gameObjectEntity1);
	gm.subscribeEntity<RenderSystem>(gameObjectEntity2);
	gm.subscribeEntity<RenderSystem>(gameObjectEntity3);


	App::initVulkan();
	App::mainLoop(gm);
	App::cleanup();
}

void App::initVulkan()
{
	App::setupGameObjects();
	for (int i = 0; i < MAX_OBJECTS; i++)
	{
		GameObject::initUniformBuffers(gameObjects[i], *vulkanDevice);
		DescriptorHandlerFactory::allocateDescriptorSets(*vulkanDevice, *descriptorHandler, gameObjects[i]);
		DescriptorHandlerFactory::updateUniformDescriptors(*vulkanDevice, gameObjects[i]);
		DescriptorHandlerFactory::updateTextureDescriptors(*vulkanDevice, gameObjects[i]);
	}
}

void App::mainLoop(GeneralManager& gm)
{
	while (!window->shouldClose())
	{
		auto start = std::chrono::high_resolution_clock::now();
		glfwPollEvents();
		gm.update(1);
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> duration = end - start;
		time += duration.count();
		frameCount++;
		if (time >= 1.0f)
		{
			std::cout << "FPS: " << frameCount << std::endl;
			frameCount = 0;
			time = 0.0f;
		}
	}
}

void App::cleanup()
{
	vulkanDevice->device.waitIdle();

	SwapChainFactory::cleanupSwapChain(*swapChain);
}

void App::setupGameObjects()
{
	const std::string TEXTURE_PATH = "assets/textures/viking_room.png";
	
	// Object 1 - Center
	gameObjects[0].position = {0.0f, 0.0f, 0.0f};
	gameObjects[0].rotation = {0.0f, 0.0f, 0.0f};
	gameObjects[0].scale = {1.0f, 1.0f, 1.0f};
	MaterialAsset::generateTextureData(TEXTURE_PATH, *vulkanDevice, gameObjects[0]);
	// Object 2 - Left
	gameObjects[1].position = {-2.0f, 0.0f, -1.0f};
	gameObjects[1].rotation = {0.0f, 0.0f, 0.0f};
	gameObjects[1].scale = {0.75f, 0.75f, 0.75f};
	MaterialAsset::generateTextureData(TEXTURE_PATH, *vulkanDevice, gameObjects[1]);
	// Object 3 - Right
	gameObjects[2].position = {2.0f, 0.0f, -1.0f};
	gameObjects[2].rotation = {0.0f, 0.0f, 0.0f};
	gameObjects[2].scale = {0.75f, 0.75f, 0.75f};
	MaterialAsset::generateTextureData(TEXTURE_PATH, *vulkanDevice, gameObjects[2]);
}