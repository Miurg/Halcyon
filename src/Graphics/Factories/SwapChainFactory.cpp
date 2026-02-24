#include "SwapChainFactory.hpp"
#include "../VulkanUtils.hpp"
#include <iostream>
#include "../Resources/Managers/DescriptorManager.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"

void SwapChainFactory::createSwapChain(SwapChain& swapChain, VulkanDevice& deviceContext, Window& window,
                                       vk::SwapchainKHR oldHandle)
{
	createSwapChainHandle(swapChain, deviceContext, window, oldHandle);
}

void SwapChainFactory::createSwapChainHandle(SwapChain& swapChain, VulkanDevice& deviceContext, Window& window,
                                             vk::SwapchainKHR oldHandle)
{
	// Query swap chain support details
	auto surfaceCapabilities = deviceContext.physicalDevice.getSurfaceCapabilitiesKHR(deviceContext.surface);
	vk::SurfaceFormatKHR swapChainSurfaceFormat =
	    chooseSwapSurfaceFormat(deviceContext.physicalDevice.getSurfaceFormatsKHR(deviceContext.surface));
	swapChain.swapChainExtent = chooseSwapExtent(surfaceCapabilities, window);
	vk::PresentModeKHR swapChainPresentMode =
	    chooseSwapPresentMode(deviceContext.physicalDevice.getSurfacePresentModesKHR(deviceContext.surface));
	swapChain.swapChainImageFormat = swapChainSurfaceFormat.format;

	// Determine number of images in the swap chain
	auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
	minImageCount = (surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount)
	                    ? surfaceCapabilities.maxImageCount
	                    : minImageCount;
	if (surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount)
	{
		minImageCount = surfaceCapabilities.maxImageCount;
	}

	vk::SwapchainCreateInfoKHR swapChainCreateInfo{};
	swapChainCreateInfo.flags = vk::SwapchainCreateFlagsKHR();
	swapChainCreateInfo.surface = deviceContext.surface;
	swapChainCreateInfo.minImageCount = minImageCount;
	swapChainCreateInfo.imageFormat = swapChain.swapChainImageFormat;
	swapChainCreateInfo.imageColorSpace = swapChainSurfaceFormat.colorSpace;
	swapChainCreateInfo.imageExtent = swapChain.swapChainExtent;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
	swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
	swapChainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
	swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	swapChainCreateInfo.presentMode =
	    chooseSwapPresentMode(deviceContext.physicalDevice.getSurfacePresentModesKHR(deviceContext.surface));
	swapChainCreateInfo.clipped = true;
	swapChainCreateInfo.oldSwapchain = oldHandle;

	swapChain.swapChainHandle = vk::raii::SwapchainKHR(deviceContext.device, swapChainCreateInfo);
	swapChain.swapChainImages = swapChain.swapChainHandle.getImages();

	createImageViews(swapChain, deviceContext, window);
}

void SwapChainFactory::createImageViews(SwapChain& swapChain, VulkanDevice& deviceContext, Window& window)
{
	swapChain.swapChainImageViews.clear();
	for (auto image : swapChain.swapChainImages)
	{
		swapChain.swapChainImageViews.emplace_back(VulkanUtils::createImageView(
		    image, swapChain.swapChainImageFormat, vk::ImageAspectFlagBits::eColor, deviceContext));
	}
}

vk::SurfaceFormatKHR
SwapChainFactory::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
		    availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			return availableFormat;
		}
	}
	return availableFormats[0];
}

vk::PresentModeKHR SwapChainFactory::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == vk::PresentModeKHR::eMailbox)
		{
			return availablePresentMode;
		}
	}
	return vk::PresentModeKHR::eFifo;
}

vk::Extent2D SwapChainFactory::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, Window& window)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}

	return {std::clamp<uint32_t>(window.height, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
	        std::clamp<uint32_t>(window.width, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
}

void SwapChainFactory::cleanupSwapChain(SwapChain& swapChain)
{
	swapChain.swapChainImageViews.clear();
	swapChain.swapChainHandle = nullptr;
}

void SwapChainFactory::recreateSwapChain(SwapChain& swapChain, VulkanDevice& device, Window& window,
                                         DescriptorManager& dManager, GlobalDSetComponent& globalDSetComponent,
                                         TextureManager& tManager)
{
	int width = 0, height = 0;
	width = window.width;
	height = window.height;
	while (width == 0 || height == 0)
	{
		width = window.width;
		height = window.height;
		window.waitEvents();
	}
	device.device.waitIdle();

	vk::SwapchainKHR oldHandle = *swapChain.swapChainHandle;

	swapChain.swapChainImageViews.clear();

	createSwapChain(swapChain, device, window, oldHandle);
	tManager.resizeTexture(swapChain.offscreenTextureHandle, swapChain.swapChainExtent.width,
	                       swapChain.swapChainExtent.height);
	dManager.updateSingleTextureDSet(globalDSetComponent.fxaaDSets, 0,
	                                 tManager.textures[swapChain.offscreenTextureHandle.id].textureImageView,
	                                 tManager.textures[swapChain.offscreenTextureHandle.id].textureSampler);
	tManager.resizeTexture(swapChain.depthTextureHandle, swapChain.swapChainExtent.width,
	                       swapChain.swapChainExtent.height);
#ifdef _DEBUG
	std::cout << "Swap chain recreated." << std::endl;
#endif
}