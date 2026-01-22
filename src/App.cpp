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
#include "Graphics/Components/FrameImageComponent.hpp"
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

	CoreInit::Run(gm);
	GraphicsInit::Run(gm);
	GameInit::gameInitStart(gm);

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
