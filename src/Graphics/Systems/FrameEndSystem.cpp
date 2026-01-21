#include "FrameEndSystem.hpp"
#include "../VulkanConst.hpp"
#include <iostream>
#include <vulkan/vulkan_raii.hpp>
#include "../Factories/SwapChainFactory.hpp"
#include "../../Core/GeneralManager.hpp"
#include "../GraphicsContexts.hpp"
#include "../Components/VulkanDeviceComponent.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../../Platform/PlatformContexts.hpp"
#include "../../Platform/Components/WindowComponent.hpp"
#include "../FrameData.hpp"
#include "../Components/FrameDataComponent.hpp"
#include "../Components/CurrentFrameComponent.hpp"
#include "../Components/FrameImageComponent.hpp"

void FrameEndSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "PresentSystem registered!" << std::endl;
}

void FrameEndSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "PresentSystem shutdown!" << std::endl;
}

void FrameEndSystem::update(float deltaTime, GeneralManager& gm)
{
	VulkanDevice& vulkanDevice =
	    *gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	SwapChain& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	Window& window = *gm.getContextComponent<MainWindowContext, WindowComponent>()->windowInstance;
	std::vector<FrameData>& framesData =
	    *gm.getContextComponent<MainFrameDataContext, FrameDataComponent>()->frameDataArray;
	CurrentFrameComponent* currentFrameComp = gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>();
	uint32_t imageIndex = gm.getContextComponent<FrameImageContext, FrameImageComponent>()->imageIndex;
	if (!currentFrameComp->frameValid) return;

	vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);

	const vk::SubmitInfo submitInfo(*framesData[currentFrameComp->currentFrame].presentCompleteSemaphore,
	                                waitDestinationStageMask, *framesData[currentFrameComp->currentFrame].commandBuffer,
	                                *framesData[currentFrameComp->currentFrame].renderFinishedSemaphore);

	vulkanDevice.graphicsQueue.submit(submitInfo, *framesData[currentFrameComp->currentFrame].inFlightFence);

	// Present the image
	const vk::PresentInfoKHR presentInfoKHR(*framesData[currentFrameComp->currentFrame].renderFinishedSemaphore,
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
}