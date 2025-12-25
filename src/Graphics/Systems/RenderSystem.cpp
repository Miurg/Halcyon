#include "RenderSystem.hpp"
#include "../VulkanConst.hpp"
#include <iostream>
#include <chrono>

#include "../Factories/SwapChainFactory.hpp"
#include "../../Core/GeneralManager.hpp"
#include "../GraphicsContexts.hpp"


void RenderSystem::processEntity(Entity entity, GeneralManager& manager, float dt) {}

void RenderSystem::recordCommandBuffer(vk::raii::CommandBuffer& commandBuffer, uint32_t imageIndex,
                                       std::vector<GameObject*>& gameObjects, SwapChain& swapChain,
                                       PipelineHandler& pipelineHandler, uint32_t currentFrame, Model& model)
{
	commandBuffer.begin({});

	transitionImageLayout(commandBuffer, swapChain.swapChainImages[imageIndex], vk::ImageLayout::eUndefined,
	                      vk::ImageLayout::eColorAttachmentOptimal, {}, vk::AccessFlagBits2::eColorAttachmentWrite,
	                      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
	                      vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::ImageAspectFlagBits::eColor);
	transitionImageLayout(commandBuffer, swapChain.depthImage, vk::ImageLayout::eUndefined,
	                      vk::ImageLayout::eDepthStencilAttachmentOptimal, {},
	                      vk::AccessFlagBits2::eDepthStencilAttachmentWrite, vk::PipelineStageFlagBits2::eTopOfPipe,
	                      vk::PipelineStageFlagBits2::eEarlyFragmentTests, vk::ImageAspectFlagBits::eDepth);

	vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
	vk::RenderingAttachmentInfo attachmentInfo;
	attachmentInfo.imageView = swapChain.swapChainImageViews[imageIndex];
	attachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	attachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
	attachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
	attachmentInfo.clearValue = clearColor;

	vk::RenderingAttachmentInfo depthAttachmentInfo;
	depthAttachmentInfo.imageView = swapChain.depthImageView;
	depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
	depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
	depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
	depthAttachmentInfo.clearValue = swapChain.clearDepth;

	vk::RenderingInfo renderingInfo;
	renderingInfo.renderArea.offset = 0;
	renderingInfo.renderArea.extent = swapChain.swapChainExtent;
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &attachmentInfo;
	renderingInfo.pDepthAttachment = &depthAttachmentInfo;

	commandBuffer.beginRendering(renderingInfo);
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineHandler.graphicsPipeline);

	commandBuffer.bindVertexBuffers(0, *model.vertexBuffer, {0});
	commandBuffer.bindIndexBuffer(*model.indexBuffer, 0, vk::IndexType::eUint32);

	commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
	                                           static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
	commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));
	for (const auto gameObject : gameObjects)
	{
		// Bind the descriptor set for this object
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineHandler.pipelineLayout, 0,
		                                  *gameObject->descriptorSets[currentFrame], nullptr);
		commandBuffer.drawIndexed(model.indices.size(), 1, 0, 0, 0);
	}
	commandBuffer.endRendering();

	transitionImageLayout(commandBuffer, swapChain.swapChainImages[imageIndex], vk::ImageLayout::eColorAttachmentOptimal,
	                      vk::ImageLayout::ePresentSrcKHR, vk::AccessFlagBits2::eColorAttachmentWrite, {},
	                      vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eBottomOfPipe,
	                      vk::ImageAspectFlagBits::eColor);

	commandBuffer.end();
}

void RenderSystem::transitionImageLayout(vk::raii::CommandBuffer& commandBuffer, vk::Image image,
                                         vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                                         vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask,
                                         vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask,
                                         vk::ImageAspectFlags imageAspectFlags)
{
	vk::ImageMemoryBarrier2 barrier;
	barrier.srcStageMask = srcStageMask;
	barrier.srcAccessMask = srcAccessMask;
	barrier.dstStageMask = dstStageMask;
	barrier.dstAccessMask = dstAccessMask;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	barrier.subresourceRange.aspectMask = imageAspectFlags;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	vk::DependencyInfo dependencyInfo;
	dependencyInfo.dependencyFlags = {};
	dependencyInfo.imageMemoryBarrierCount = 1;
	dependencyInfo.pImageMemoryBarriers = &barrier;
	commandBuffer.pipelineBarrier2(dependencyInfo);
}

void RenderSystem::update(float deltaTime, GeneralManager& gm, const std::vector<Entity>& entities)
{
	VulkanDevice& vulkanDevice = *gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	SwapChain& swapChain = *gm.getContextComponent<MainSwapChainContext, SwapChainComponent>()->swapChainInstance;
	PipelineHandler& pipelineHandler = *gm.getContextComponent<MainSignatureContext, PipelineHandlerComponent>()->pipelineHandler;
	Window& window = *gm.getContextComponent<MainWindowContext, WindowComponent>()->windowInstance;
	Model& model = *gm.getContextComponent<MainModelContext, ModelHandlerComponent>()->modelInstance;
	std::vector<FrameData>& framesData =
	    *gm.getContextComponent<MainFrameDataContext, FrameDataComponent>()->frameDataArray;
	uint32_t currentFrame = gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>()->currentFrame;
	CameraComponent* mainCamera = gm.getContextComponent<MainCameraContext, CameraComponent>();

	std::vector<GameObject*> gameObjects;
	for (int i = 0; i < entities.size(); i++)
	{
		auto gameObjectComponent = gm.getComponent<GameObjectComponent>(entities[i]);
		gameObjects.push_back(gameObjectComponent->objectInstance);
	}


	while (vk::Result::eTimeout ==
	       vulkanDevice.device.waitForFences(*framesData[currentFrame].inFlightFence, vk::True, UINT64_MAX));

	// Handle window resize
	if (window.framebufferResized)
	{
		SwapChainFactory::recreateSwapChain(swapChain, vulkanDevice, window);
		window.framebufferResized = false;
		return;
	}

	// Acquire the next image from the swap chain
	auto [result, imageIndex] = swapChain.swapChainHandle.acquireNextImage(
	    UINT64_MAX, *framesData[currentFrame].presentCompleteSemaphore, nullptr);
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

	vulkanDevice.device.resetFences(*framesData[currentFrame].inFlightFence);
	framesData[currentFrame].commandBuffer.reset();

	RenderSystem::updateUniformBuffer(currentFrame, gameObjects, swapChain, currentFrame, mainCamera);

	recordCommandBuffer(framesData[currentFrame].commandBuffer, imageIndex, gameObjects, swapChain, pipelineHandler, currentFrame, model);

	vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);

	const vk::SubmitInfo submitInfo(*framesData[currentFrame].presentCompleteSemaphore, waitDestinationStageMask,
	                                *framesData[currentFrame].commandBuffer,
	                                *framesData[imageIndex].renderFinishedSemaphore);

	vulkanDevice.graphicsQueue.submit(submitInfo, *framesData[currentFrame].inFlightFence);

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

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void RenderSystem::updateUniformBuffer(uint32_t currentImage, std::vector<GameObject*>& gameObjects,
                                       SwapChain& swapChain, uint32_t currentFrame, CameraComponent* mainCamera)
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	glm::mat4 view = mainCamera->getViewMatrix();
	glm::mat4 proj = glm::perspective(glm::radians(mainCamera->fov),
	                                  static_cast<float>(swapChain.swapChainExtent.width) /
	                                      static_cast<float>(swapChain.swapChainExtent.height),
	                                  0.1f, 2000.0f);
	proj[1][1] *= -1; // Flip Y for Vulkan

	for (auto gameObject : gameObjects)
	{
		// Get the model matrix for this object
		glm::mat4 rotation =
		    glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 initialRotation = glm::rotate(glm::mat4(1.0f), time/10 * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 model = gameObject->getModelMatrix() * rotation * initialRotation;

		// Create and update the UBO
		UniformBufferObject ubo{.model = model, .view = view, .proj = proj};
		memcpy(gameObject->uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
	}
}