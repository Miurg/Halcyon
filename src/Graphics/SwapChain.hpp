#pragma once

#include <vulkan/vulkan_raii.hpp>

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
	vk::Format depthFormat = vk::Format::eUndefined;

	std::vector<vk::Image> swapChainImages;
	std::vector<vk::raii::ImageView> swapChainImageViews;
};