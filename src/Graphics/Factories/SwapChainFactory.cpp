#include "SwapChainFactory.hpp"
#include "../VulkanUtils.hpp"
#include <iostream>

void SwapChainFactory::createSwapChain(SwapChain& swapChain, VulkanDevice& deviceContext, Window& window,
                                       vk::SwapchainKHR oldHandle)
{
	createSwapChainHandle(swapChain, deviceContext, window, oldHandle);
	createImageViews(swapChain, deviceContext, window);
	createDepthResources(swapChain, deviceContext, window);
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
}

void SwapChainFactory::createImageViews(SwapChain& swapChain, VulkanDevice& deviceContext, Window& window)
{
	swapChain.swapChainImageViews.clear();
	for (auto image : swapChain.swapChainImages)
	{
		swapChain.swapChainImageViews.emplace_back(
		    VulkanUtils::createImageView(image, swapChain.swapChainImageFormat, vk::ImageAspectFlagBits::eColor, deviceContext));
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

void SwapChainFactory::recreateSwapChain(SwapChain& swapChain, VulkanDevice& device, Window& window)
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

#ifdef _DEBUG
	std::cout << "Swap chain recreated." << std::endl;
#endif
}

void SwapChainFactory::createDepthResources(SwapChain& swapChain, VulkanDevice& device, Window& window)
{
	swapChain.depthFormat = findDepthFormat(device);
	VulkanUtils::createImage(swapChain.swapChainExtent.width, swapChain.swapChainExtent.height, swapChain.depthFormat, vk::ImageTiling::eOptimal,
	                         vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal,
	                         swapChain.depthImage, swapChain.depthImageMemory, device);
	swapChain.depthImageView =
	    VulkanUtils::createImageView(swapChain.depthImage, swapChain.depthFormat, vk::ImageAspectFlagBits::eDepth, device);
}

vk::Format SwapChainFactory::findDepthFormat(VulkanDevice& device)
{
	return findSupportedFormat(device, 
		{vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint}, 
		vk::ImageTiling::eOptimal,
	    vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

vk::Format SwapChainFactory::findSupportedFormat(VulkanDevice& device, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
                                          vk::FormatFeatureFlags features)
{
	for (const auto format : candidates)
	{
		vk::FormatProperties props = device.physicalDevice.getFormatProperties(format);
		if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

void SwapChainFactory::createOffscreenResources(SwapChain& swapChain, VulkanDevice& device, Window& window)
{
	VulkanUtils::createImage(
	    swapChain.swapChainExtent.width, swapChain.swapChainExtent.height, swapChain.swapChainImageFormat,
	    vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
	    vk::MemoryPropertyFlagBits::eDeviceLocal, swapChain.offscreenImage, swapChain.offscreenImageMemory, device);

	swapChain.offscreenImageView = VulkanUtils::createImageView(swapChain.offscreenImage, swapChain.swapChainImageFormat,
	                                                            vk::ImageAspectFlagBits::eColor, device);
	vk::SamplerCreateInfo samplerInfo{};
	samplerInfo.magFilter = vk::Filter::eLinear;
	samplerInfo.minFilter = vk::Filter::eLinear;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.anisotropyEnable = vk::False;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
	samplerInfo.unnormalizedCoordinates = vk::False;
	samplerInfo.compareEnable = vk::False;
	samplerInfo.compareOp = vk::CompareOp::eAlways;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 1.0f;

	swapChain.offscreenSampler = vk::raii::Sampler(device.device, samplerInfo);
}