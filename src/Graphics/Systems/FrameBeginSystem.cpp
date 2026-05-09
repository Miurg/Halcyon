#include "FrameBeginSystem.hpp"
#include <iostream>
#include <vulkan/vulkan_raii.hpp>
#include "../Factories/SwapChainFactory.hpp"
#include "../GraphicsContexts.hpp"
#include "../Components/VulkanDeviceComponent.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../../Platform/PlatformContexts.hpp"
#include "../../Platform/Components/WindowComponent.hpp"
#include "../FrameData.hpp"
#include "../Components/FrameDataComponent.hpp"
#include "../Components/CurrentFrameComponent.hpp"
#include "../Components/FrameImageComponent.hpp"
#include "../Managers/FrameManager.hpp"
#include "../Components/FrameManagerComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"
#include "../Components/RenderGraphComponent.hpp"
#include "../RenderGraph/RenderGraph.hpp"
#include "../Components/GraphicsSettingsComponent.hpp"

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
	DescriptorManager* dManager =
	    gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	GlobalDSetComponent* globalDSetComponent = gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	RenderGraph* rg = gm.getContextComponent<RenderGraphContext, RenderGraphComponent>()->renderGraph;
	GraphicsSettingsComponent* settings =
	    gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>();

	vulkanDevice.device.waitForFences(*frameManager->frames[currentFrameComp->currentFrame].inFlightFence, vk::True,
	                                  UINT64_MAX);
	// Handle window resize
	if (window.framebufferResized)
	{
		SwapChainFactory::recreateSwapChain(swapChain, vulkanDevice, window, *rg, *dManager, *globalDSetComponent,
		                                    *settings);
		window.framebufferResized = false;
		return;
	}

	// Acquire the next image from the swap chain
	auto [result, imageIndex] = swapChain.swapChainHandle.acquireNextImage(
	    UINT64_MAX, *frameManager->frames[currentFrameComp->currentFrame].presentCompleteSemaphore, nullptr);
	try
	{
		if (result == vk::Result::eErrorOutOfDateKHR)
		{
			window.framebufferResized = false;
			SwapChainFactory::recreateSwapChain(swapChain, vulkanDevice, window, *rg, *dManager, *globalDSetComponent,
			                                    *settings);
			return;
		}
	}
	catch (vk::OutOfDateKHRError&)
	{
		SwapChainFactory::recreateSwapChain(swapChain, vulkanDevice, window, *rg, *dManager, *globalDSetComponent,
		                                    *settings);
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
	
	currentFrameComp->frameValid = true;
	rg->clearFrame();
}