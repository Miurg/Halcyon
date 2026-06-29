#include "GraphicsCore/Systems/FrameEndSystem.hpp"
#include "GraphicsCore/VulkanConst.hpp"
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
#include "GraphicsCore/Components/LightProbeGridComponent.hpp"
#include "../GIBaker/LightProbeGIBaking.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

void FrameEndSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "FrameEndSystem registered!" << std::endl;
}

void FrameEndSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "FrameEndSystem shutdown!" << std::endl;
}

void FrameEndSystem::update(GeneralManager& gm)
{
#ifdef TRACY_ENABLE
	ZoneScopedN("FrameEndSystem");
#endif

	VulkanDevice& vulkanDevice =
	    *gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	SwapChain& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	Window& window = *gm.getContextComponent<MainWindowContext, WindowComponent>()->windowInstance;
	FrameManager* frameManager = gm.getContextComponent<FrameManagerContext, FrameManagerComponent>()->frameManager;
	CurrentFrameComponent* currentFrameComp = gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>();
	uint32_t imageIndex = gm.getContextComponent<FrameImageContext, FrameImageComponent>()->imageIndex;

	if (!currentFrameComp->frameValid) return;


	{
#ifdef TRACY_ENABLE
		ZoneScopedN("RenderGraph");
#endif
		RenderGraph* rg = gm.getContextComponent<RenderGraphContext, RenderGraphComponent>()->renderGraph;
		rg->compile();
		rg->execute(frameManager->frames[currentFrameComp->currentFrame].commandBuffer);

	}

	vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);

	const vk::SubmitInfo submitInfo(*frameManager->frames[currentFrameComp->currentFrame].presentCompleteSemaphore,
	                                waitDestinationStageMask,
	                                *frameManager->frames[currentFrameComp->currentFrame].commandBuffer,
	                                *swapChain.renderFinishedSemaphores[imageIndex]);

	vulkanDevice.graphicsQueue.submit(submitInfo, *frameManager->frames[currentFrameComp->currentFrame].inFlightFence);

	// Present the image
	const vk::PresentInfoKHR presentInfoKHR(*swapChain.renderFinishedSemaphores[imageIndex],
	                                        *swapChain.swapChainHandle, imageIndex);
	vk::Result presentResult;
	try
	{
		presentResult = vulkanDevice.presentQueue.presentKHR(presentInfoKHR);
	}
	catch (vk::OutOfDateKHRError&)
	{
		presentResult = vk::Result::eErrorOutOfDateKHR;
	}
	catch (vk::SystemError& e)
	{
		throw std::runtime_error("failed to present swap chain image!");
	}

	LightProbeGridComponent* probesGrid = gm.getContextComponent<LightProbeGridContext, LightProbeGridComponent>();
	// Rebake grid if need.
	if (probesGrid != nullptr && probesGrid->needBake)
	{
		if (probesGrid->count.x + probesGrid->count.y + probesGrid->count.z > 0)
		{
			LightProbeGIBaking::bakeAll(gm);
			probesGrid->needBake = false;
		}
	}

	// Check the result of the presentation
	if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR)
	{
		window.framebufferResized = false;
		SwapChainFactory::recreateSwapChain(swapChain, vulkanDevice, window);
	}
	else if (presentResult != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to present swap chain image!");
	}

	currentFrameComp->currentFrame = (currentFrameComp->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	currentFrameComp->frameNumber++;
}