#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#include "Graphics/DescriptorHandler.hpp"
#include "Graphics/FrameData.hpp"
#include "Graphics/GameObject.hpp"
#include "Graphics/PipelineHandler.hpp"
#include "Platform/Window.hpp"
#include "Graphics/SwapChain.hpp"
#include "Graphics/VulkanDevice.hpp"
#include "Core/GeneralManager.hpp"
#include "Graphics/Resources/Managers/AssetManager.hpp"

class App
{
public:
	App();
	void run();

private:
	Window* window;
	VulkanDevice* vulkanDevice;
	SwapChain* swapChain;
	AssetManager* assetManager;
	PipelineHandler* pipelineHandler;
	DescriptorHandler* descriptorHandler;
	std::vector<FrameData>* framesData;

	uint32_t frameCount = 0;
	float time = 0;

	std::vector<GameObject> gameObjects;

	void mainLoop(GeneralManager& gm);
	void cleanup();
	void setupGameObjects(GeneralManager& gm);
};
