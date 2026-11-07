#include "GraphicsCore/Systems/FrameBeginSystem.hpp"
#include <iostream>
#include <vulkan/vulkan_raii.hpp>
#include "../Factories/SwapChainFactory.hpp"
#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/Components/VulkanDeviceComponent.hpp"
#include "GraphicsCore/Components/SwapChainComponent.hpp"
#include "PlatformCore/PlatformContexts.hpp"
#include "PlatformCore/Components/WindowComponent.hpp"
#include "GraphicsCore/FrameData.hpp"
#include "GraphicsCore/Components/FrameDataComponent.hpp"
#include "GraphicsCore/Components/CurrentFrameComponent.hpp"
#include "GraphicsCore/Components/FrameImageComponent.hpp"
#include "../Managers/FrameManager.hpp"
#include "GraphicsCore/Components/FrameManagerComponent.hpp"
#include "GraphicsCore/Components/RenderGraphComponent.hpp"
#include "GraphicsCore/RenderGraph/RenderGraph.hpp"
#include "GraphicsCore/Components/TextureManagerComponent.hpp"
#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Components/MaterialManagerComponent.hpp"
#include "GraphicsCore/Resources/Managers/MaterialManager.hpp"
#include "GraphicsCore/Components/RenderAssetManagerComponent.hpp"
#include "GraphicsCore/Resources/Managers/RenderAssetManager.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

void FrameBeginSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "FrameBeginSystem registered!" << std::endl;
}

void FrameBeginSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "FrameBeginSystem shutdown!" << std::endl;
}

void FrameBeginSystem::update(GeneralManager& gm)
{
#ifdef TRACY_ENABLE
	ZoneScopedN("FrameBeginSystem");
#endif

	VulkanDevice& vulkanDevice =
	    *gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	SwapChain& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	Window& window = *gm.getContextComponent<MainWindowContext, WindowComponent>()->windowInstance;
	FrameManager* frameManager = gm.getContextComponent<FrameManagerContext, FrameManagerComponent>()->frameManager;
	CurrentFrameComponent* currentFrameComp = gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>();
	currentFrameComp->frameValid = false;
	RenderGraph* rg = gm.getContextComponent<RenderGraphContext, RenderGraphComponent>()->renderGraph;

	{
#ifdef TRACY_ENABLE
		ZoneScopedN("waitForFences");
#endif
		vk::Result fenceResult = vulkanDevice.device.waitForFences(
		    *frameManager->frames[currentFrameComp->currentFrame].inFlightFence, vk::True, UINT64_MAX);

		switch (fenceResult)
		{
		case vk::Result::eSuccess:
			break;

		case vk::Result::eTimeout:
			std::cerr << "[FrameBeginSystem] waitForFences timed out. Skipping frame." << std::endl;
			return;

		case vk::Result::eErrorDeviceLost:
			std::cerr << "[FrameBeginSystem] Device lost while waiting for fence!" << std::endl;
			std::abort();

		default:
			std::cerr << "[FrameBeginSystem] waitForFences returned unexpected error!" << std::endl;
			std::abort();
		}
	}

	// Must run after the fence wait: only then is it safe to reclaim retired resources.
	TextureManager& textureManager =
	    *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	textureManager.collectTextureFrees(currentFrameComp->frameNumber);
	MaterialManager& materialManager =
	    *gm.getContextComponent<MaterialManagerContext, MaterialManagerComponent>()->materialManager;
	materialManager.collectMaterialFrees(currentFrameComp->frameNumber);

	RenderAssetManager& renderAssetManager =
	    *gm.getContextComponent<RenderAssetManagerContext, RenderAssetManagerComponent>()->renderAssetManager;
	renderAssetManager.collectGeometryFrees(currentFrameComp->frameNumber);

	// Handle window resize
	if (window.framebufferResized)
	{
		SwapChainFactory::recreateSwapChain(swapChain, vulkanDevice, window);
		window.framebufferResized = false;
		return;
	}

	// Acquire the next image from the swap chain
	{
#ifdef TRACY_ENABLE
		ZoneScopedN("acquireNextImage");
#endif
		auto [result, imageIndex] = swapChain.swapChainHandle.acquireNextImage(
		    UINT64_MAX, *frameManager->frames[currentFrameComp->currentFrame].presentCompleteSemaphore, nullptr);

		if (result == vk::Result::eSuccess || result == vk::Result::eSuboptimalKHR)
		{
			currentFrameComp->frameValid = true;
		}
		else if (result == vk::Result::eErrorOutOfDateKHR)
		{
			window.framebufferResized = false;
			SwapChainFactory::recreateSwapChain(swapChain, vulkanDevice, window);
			return;
		}
		else if (result == vk::Result::eTimeout || result == vk::Result::eNotReady)
		{
			currentFrameComp->frameValid = false;
			return;
		}
		else
		{
			std::cout << "failed to acquire swap chain image!" << std::endl;
			std::abort();
		}

		vulkanDevice.device.resetFences(*frameManager->frames[currentFrameComp->currentFrame].inFlightFence);
		frameManager->frames[currentFrameComp->currentFrame].commandBuffer.reset();

		FrameImageComponent* frameImageComponent = gm.getContextComponent<FrameImageContext, FrameImageComponent>();
		frameImageComponent->imageIndex = imageIndex;
	}

	currentFrameComp->frameValid = true;
	rg->clearFrame();
}
