#include "RenderSystem.hpp"
#include "../VulkanConst.hpp"
#include <iostream>
#include <chrono>
#include <vulkan/vulkan_raii.hpp>
#include "../Factories/SwapChainFactory.hpp"
#include "../../Core/GeneralManager.hpp"
#include "../GraphicsContexts.hpp"
#include "../Factories/CommandBufferFactory.hpp"
#include "../Components/VulkanDeviceComponent.hpp"
#include "../Components/PipelineHandlerComponent.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../../Platform/PlatformContexts.hpp"
#include "../../Platform/Components/WindowComponent.hpp"
#include "../Components/BufferManagerComponent.hpp"
#include "../FrameData.hpp"
#include "../Components/FrameDataComponent.hpp"
#include "../Components/CurrentFrameComponent.hpp"

void RenderSystem::processEntity(Entity entity, GeneralManager& manager, float dt) {}

void RenderSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "RenderSystem registered!" << std::endl;
}

void RenderSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "RenderSystem shutdown!" << std::endl;
}

void RenderSystem::update(float deltaTime, GeneralManager& gm, const std::vector<Entity>& entities)
{
	VulkanDevice& vulkanDevice =
	    *gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	SwapChain& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	PipelineHandler& pipelineHandler =
	    *gm.getContextComponent<MainSignatureContext, PipelineHandlerComponent>()->pipelineHandler;
	Window& window = *gm.getContextComponent<MainWindowContext, WindowComponent>()->windowInstance;
	BufferManager& bufferManager =
	    *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	std::vector<FrameData>& framesData =
	    *gm.getContextComponent<MainFrameDataContext, FrameDataComponent>()->frameDataArray;
	CurrentFrameComponent* currentFrameComp = gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>();
	CameraComponent* mainCamera = gm.getContextComponent<MainCameraContext, CameraComponent>();
	CameraComponent* sunCamera = gm.getContextComponent<LightCameraContext, CameraComponent>();
	LightComponent* lightTexture = gm.getContextComponent<MainCameraContext, LightComponent>();

	std::vector<int> textureInfo;
	std::vector<MeshInfoComponent*> meshInfos;
	for (int i = 0; i < entities.size(); i++)
	{
		meshInfos.push_back(gm.getComponent<MeshInfoComponent>(entities[i]));
		auto textureInfoComponent = gm.getComponent<TextureInfoComponent>(entities[i]);
		textureInfo.push_back(textureInfoComponent->textureIndex);
	}
	ModelsBuffersComponent* ssbos = gm.getContextComponent<ModelSSBOsContext, ModelsBuffersComponent>();

	while (vk::Result::eTimeout == vulkanDevice.device.waitForFences(
	                                   *framesData[currentFrameComp->currentFrame].inFlightFence, vk::True, UINT64_MAX));

	// Handle window resize
	if (window.framebufferResized)
	{
		SwapChainFactory::recreateSwapChain(swapChain, vulkanDevice, window);
		window.framebufferResized = false;
		return;
	}

	// Acquire the next image from the swap chain
	auto [result, imageIndex] = swapChain.swapChainHandle.acquireNextImage(
	    UINT64_MAX, *framesData[currentFrameComp->currentFrame].presentCompleteSemaphore, nullptr);
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

	vulkanDevice.device.resetFences(*framesData[currentFrameComp->currentFrame].inFlightFence);
	framesData[currentFrameComp->currentFrame].commandBuffer.reset();

	CommandBufferFactory::recordCommandBuffer(framesData[currentFrameComp->currentFrame].commandBuffer, imageIndex,
	                                          textureInfo, swapChain, pipelineHandler, currentFrameComp->currentFrame,
	                                          bufferManager, meshInfos, *mainCamera, *ssbos, *sunCamera, *lightTexture);

	vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);

	const vk::SubmitInfo submitInfo(*framesData[currentFrameComp->currentFrame].presentCompleteSemaphore,
	                                waitDestinationStageMask, *framesData[currentFrameComp->currentFrame].commandBuffer,
	                                *framesData[imageIndex].renderFinishedSemaphore);

	vulkanDevice.graphicsQueue.submit(submitInfo, *framesData[currentFrameComp->currentFrame].inFlightFence);

	// Present the image
	const vk::PresentInfoKHR presentInfoKHR(*framesData[imageIndex].renderFinishedSemaphore, *swapChain.swapChainHandle,
	                                        imageIndex);
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