#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "Resources/Managers/ResourceHandles.hpp"

class SwapChain
{
public:
	vk::Extent2D swapChainExtent = vk::Extent2D();
	vk::Format swapChainImageFormat = vk::Format::eUndefined;
	vk::raii::SwapchainKHR swapChainHandle = nullptr;

	TextureHandle depthTextureHandle;

	std::vector<vk::Image> swapChainImages;
	std::vector<vk::raii::ImageView> swapChainImageViews;

	TextureHandle offscreenTextureHandle;
	TextureHandle viewNormalsTextureHandle;
	TextureHandle ssaoTextureHandle;
	TextureHandle ssaoBlurTextureHandle;
	TextureHandle ssaoNoiseTextureHandle;
	vk::Format hdrFormat = vk::Format::eR16G16B16A16Sfloat;
};