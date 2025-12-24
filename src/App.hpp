#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32

// Include Vulkan RAII headers
#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "Graphics/DescriptorHandler.hpp"
#include "Graphics/FrameData.hpp"
#include "Graphics/GameObject.hpp"
#include "Graphics/Model.hpp"
#include "Graphics/PipelineHandler.hpp"
#include "Platform/Window.hpp"
#include "Graphics/Systems/RenderSystem.hpp"
#include "Graphics/SwapChain.hpp"
#include "Graphics/VulkanConst.hpp"
#include "Graphics/VulkanDevice.hpp"
#include "Core/GeneralManager.hpp"
#include "Graphics/FrameData.hpp"

class App
{
public:
	App();
	void run();

private:
	Window* window;
	VulkanDevice* vulkanDevice;
	SwapChain* swapChain;
	Model* model;
	PipelineHandler* pipelineHandler;
	DescriptorHandler* descriptorHandler;
	RenderSystem* renderSystem;
	std::vector<FrameData>* framesData;

	uint32_t frameCount = 0;
	float time = 0;

	std::array<GameObject, MAX_OBJECTS> gameObjects;

	void initVulkan();
	void mainLoop(GeneralManager& gm);
	void cleanup();
	void setupGameObjects();
};
