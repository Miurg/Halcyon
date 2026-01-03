#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "../VulkanDevice.hpp"
#include "../../Platform/Window.hpp"
#include "../SwapChain.hpp"

class SwapChainFactory
{
public:
	static void createSwapChain(SwapChain& swapChain, VulkanDevice& device,
	                            Window& window, vk::SwapchainKHR oldHandle = nullptr);
	static void createSwapChainHandle(SwapChain& swapChain, VulkanDevice& device, Window& window,
	                                  vk::SwapchainKHR oldHandle = nullptr);
	static void createImageViews(SwapChain& swapChain, VulkanDevice& device, Window& window);
	static void cleanupSwapChain(SwapChain& swapChain);
	static void recreateSwapChain(SwapChain& swapChain, VulkanDevice& device, Window& window);
	static void createDepthResources(SwapChain& swapChain, VulkanDevice& device, Window& window);
	static vk::Format findSupportedFormat(VulkanDevice& device, const std::vector<vk::Format>& candidates,
	                                      vk::ImageTiling tiling, vk::FormatFeatureFlags features);
	static vk::Format findDepthFormat(VulkanDevice& device);

	static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
	static vk::PresentModeKHR
	chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
	static vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, Window& window);
	static void createShadowResources(SwapChain& swapChain, VulkanDevice& device);
};