#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#include "Graphics/FrameData.hpp"
#include "Graphics/PipelineHandler.hpp"
#include "Platform/Window.hpp"
#include "Graphics/SwapChain.hpp"
#include "Graphics/VulkanDevice.hpp"
#include "Core/GeneralManager.hpp"
#include "Graphics/Resources/Managers/BufferManager.hpp"

class App
{
public:
	App();
	void run();

private:
	Window* window;
	VulkanDevice* vulkanDevice;
	SwapChain* swapChain;
	BufferManager* bufferManager;
	PipelineHandler* pipelineHandler;
	DescriptorManager* descriptorManager;
	std::vector<FrameData>* framesData;

	uint32_t frameCount = 0;
	float time = 0;

	void mainLoop(GeneralManager& gm);
	void cleanup();
};
