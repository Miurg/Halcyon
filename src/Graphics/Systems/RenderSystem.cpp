#include "RenderSystem.hpp"
#include "../VulkanConst.hpp"
#include <iostream>
#include <chrono>

#include "../Factories/SwapChainFactory.hpp"
#include "../../Core/GeneralManager.hpp"
#include "../GraphicsContexts.hpp"
#include "../Factories/CommandBufferFactory.hpp"


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
	VulkanDevice& vulkanDevice = *gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	SwapChain& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	PipelineHandler& pipelineHandler = *gm.getContextComponent<MainSignatureContext, PipelineHandlerComponent>()->pipelineHandler;
	Window& window = *gm.getContextComponent<MainWindowContext, WindowComponent>()->windowInstance;
	AssetManager& assetManager =
	    *gm.getContextComponent<AssetManagerContext, AssetManagerComponent>()->assetManager;
	std::vector<FrameData>& framesData =
	    *gm.getContextComponent<MainFrameDataContext, FrameDataComponent>()->frameDataArray;
	CurrentFrameComponent* currentFrameComp = gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>();
	CameraComponent* mainCamera = gm.getContextComponent<MainCameraContext, CameraComponent>();

	std::vector<GameObject*> gameObjects;
	std::vector<TransformComponent*> transforms;
	std::vector<MeshInfoComponent*> meshInfos;
	for (int i = 0; i < entities.size(); i++)
	{
		meshInfos.push_back(gm.getComponent<MeshInfoComponent>(entities[i]));
		transforms.push_back(gm.getComponent<TransformComponent>(entities[i]));
		auto gameObjectComponent = gm.getComponent<GameObjectComponent>(entities[i]);
		gameObjects.push_back(gameObjectComponent->objectInstance);
	}

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

	RenderSystem::updateUniformBuffer(currentFrameComp->currentFrame, gameObjects, swapChain,
	                                  currentFrameComp->currentFrame, mainCamera, transforms);

	CommandBufferFactory::recordCommandBuffer(framesData[currentFrameComp->currentFrame].commandBuffer, imageIndex,
	                                          gameObjects, swapChain, pipelineHandler, currentFrameComp->currentFrame,
	                                          assetManager, meshInfos, *mainCamera);

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

void RenderSystem::updateUniformBuffer(uint32_t currentImage, std::vector<GameObject*>& gameObjects,
                                       SwapChain& swapChain, uint32_t currentFrame, CameraComponent* mainCamera,
                                       std::vector<TransformComponent*>& transforms)
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	glm::mat4 view = mainCamera->getViewMatrix();
	glm::mat4 proj = glm::perspective(glm::radians(mainCamera->fov),
	                                  static_cast<float>(swapChain.swapChainExtent.width) /
	                                      static_cast<float>(swapChain.swapChainExtent.height),
	                                  0.1f, 2000.0f);
	proj[1][1] *= -1; // Vulkan Y flip

	CameraUBO cameraUbo{.view = view, .proj = proj};
	memcpy(mainCamera->cameraBuffersMapped[currentFrame], &cameraUbo, sizeof(cameraUbo));

	for (size_t i = 0; i < gameObjects.size(); i++)
	{
		glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 initialRotation =
		    glm::rotate(glm::mat4(1.0f), time / 10 * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		glm::mat4 model = transforms[i]->getModelMatrix() * rotation * initialRotation;
		ModelUBO modelUbo{.model = model};

		memcpy(gameObjects[i]->modelBuffersMapped[currentFrame], &modelUbo, sizeof(modelUbo));
	}
}