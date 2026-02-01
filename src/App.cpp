#include "App.hpp"

#include <iostream>
#define GLM_FORCE_SIMD_AVX2
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <GLFW/glfw3.h>
#include "CoreInit.hpp"
#include "Platform/Components/WindowComponent.hpp"
#include "Platform/PlatformContexts.hpp"
#include "Game/GameInit.hpp"
#include "Graphics/GraphicsInit.hpp"

namespace
{
float deltaTime = 0.0f;
float lastFrame = 0.0f;
} // anonymous namespace

App::App() {}

void App::run()
{
	GeneralManager gm;
	try
	{
		CoreInit::Run(gm);
		GraphicsInit::Run(gm);
		GameInit::gameInitStart(gm);
	}
	catch (const std::exception& e)
	{
		std::cerr << "ERROR::APP::RUN::Exception: " << e.what() << std::endl;
		return;
	}
	

	App::mainLoop(gm);
	App::cleanup();
}

void App::mainLoop(GeneralManager& gm)
{
	Window* window = gm.getContextComponent<MainWindowContext, WindowComponent>()->windowInstance;
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

	//vulkanDevice->device.waitIdle();

	//SwapChainFactory::cleanupSwapChain(*swapChain);
	//bManager->~BufferManager();
}
