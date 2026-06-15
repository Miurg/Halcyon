#pragma once

#include "HalcyonExport.hpp"
#include <vulkan/vulkan_raii.hpp>

class HALCYON_API SwapChain
{
public:
	vk::Extent2D swapChainExtent = vk::Extent2D();
	vk::Format swapChainImageFormat = vk::Format::eUndefined;
	vk::raii::SwapchainKHR swapChainHandle = nullptr;
	std::vector<vk::Image> swapChainImages;
	std::vector<vk::raii::ImageView> swapChainImageViews;
	std::vector<vk::raii::Semaphore> renderFinishedSemaphores;

	vk::Format hdrFormat = vk::Format::eR16G16B16A16Sfloat;
};