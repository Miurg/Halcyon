#include "App.hpp"

#include <iostream>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLFW/glfw3.h>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Core/GeneralManager.hpp"
#include "CoreInit.hpp"
#include "Graphics/VulkanUtils.hpp"
#include "Graphics/MaterialAsset.hpp"
#include "Graphics/Factories/VulkanDeviceFactory.hpp"
#include "Graphics/Factories/SwapChainFactory.hpp"
#include "Graphics/Factories/DescriptorHandlerFactory.hpp"

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

	window = new Window(ScreenWidth, ScreenHeight, "Halcyon");

	vulkanDevice = new VulkanDevice();
	VulkanDeviceFactory::createVulkanDevice(window, vulkanDevice);
	
	swapChain = new SwapChain();
	SwapChainFactory::createSwapChain(swapChain, vulkanDevice, window);

	model = new Model(*vulkanDevice);

	descriptorHandler = new DescriptorHandler();
	DescriptorHandlerFactory::createDescriptorSetLayout(*vulkanDevice, *descriptorHandler);
	DescriptorHandlerFactory::createDescriptorPool(*vulkanDevice, *descriptorHandler);

	pipelineManager = new PipelineManager(*vulkanDevice, *swapChain, *descriptorHandler);
	renderSystem = new RenderSystem(*vulkanDevice, *swapChain, *pipelineManager, *model, *window);

	App::initVulkan();
	App::mainLoop();
	App::cleanup();
}

void App::initVulkan()
{
	App::setupGameObjects();
	for (int i = 0; i < MAX_OBJECTS; i++)
	{
		GameObject::initUniformBuffers(gameObjects[i], *vulkanDevice);
		DescriptorHandlerFactory::createDescriptorSets(*vulkanDevice, *descriptorHandler, gameObjects[i]);
	}
}

void App::mainLoop()
{
	while (!window->shouldClose())
	{
		auto start = std::chrono::high_resolution_clock::now();
		glfwPollEvents();
		renderSystem->drawFrame(gameObjects);
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

	SwapChainFactory::cleanupSwapChain(swapChain);
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