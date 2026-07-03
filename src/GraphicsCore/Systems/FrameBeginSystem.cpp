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
#include "GraphicsCore/Components/ModelManagerComponent.hpp"
#include "GraphicsCore/Resources/Managers/ModelManager.hpp"

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
		try
		{
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
					throw std::runtime_error("[FrameBeginSystem] Device lost while waiting for fence!");

				default:
					throw std::runtime_error("[FrameBeginSystem] waitForFences returned unexpected error!");
			}
		}
		catch (vk::SystemError& e)
		{
			throw std::runtime_error(
			    std::string("[FrameBeginSystem] waitForFences failed with exception: ") + e.what());
		}
	}

	// Must run after the fence wait: only then is it safe to reclaim retired resources.
	TextureManager& textureManager =
	    *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	textureManager.collectTextureFrees(currentFrameComp->frameNumber);

	ModelManager& modelManager =
	    *gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;
	modelManager.collectGeometryFrees(currentFrameComp->frameNumber);

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

		try
		{
			if (result == vk::Result::eErrorOutOfDateKHR)
			{
				window.framebufferResized = false;
				SwapChainFactory::recreateSwapChain(swapChain, vulkanDevice, window);
				return;
			}
		}
		catch (vk::OutOfDateKHRError&)
		{
			SwapChainFactory::recreateSwapChain(swapChain, vulkanDevice, window);
			return;
		}
		catch (vk::SystemError& e)
		{
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		vulkanDevice.device.resetFences(*frameManager->frames[currentFrameComp->currentFrame].inFlightFence);
		frameManager->frames[currentFrameComp->currentFrame].commandBuffer.reset();

		FrameImageComponent* frameImageComponent = gm.getContextComponent<FrameImageContext, FrameImageComponent>();
		frameImageComponent->imageIndex = imageIndex;
	}

	currentFrameComp->frameValid = true;
	rg->clearFrame();
}