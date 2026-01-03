#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "VulkanDevice.hpp"
#include "../Platform/Window.hpp"

class SwapChain
{
public:
	vk::Extent2D swapChainExtent = vk::Extent2D();
	vk::Format swapChainImageFormat = vk::Format::eUndefined;
	vk::raii::SwapchainKHR swapChainHandle = nullptr;

	vk::raii::Image depthImage = nullptr;
	vk::raii::DeviceMemory depthImageMemory = nullptr;
	vk::raii::ImageView depthImageView = nullptr;
	vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
	vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);
	vk::Format depthFormat;

	std::vector<vk::Image> swapChainImages;
	std::vector<vk::raii::ImageView> swapChainImageViews;

	vk::raii::Image shadowImage = nullptr;
	vk::raii::DeviceMemory shadowImageMemory = nullptr;
	vk::raii::ImageView shadowImageView = nullptr;
	vk::raii::Sampler shadowSampler = nullptr;
};