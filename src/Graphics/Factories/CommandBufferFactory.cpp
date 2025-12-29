#include "CommandBufferFactory.hpp"

void CommandBufferFactory::recordCommandBuffer(vk::raii::CommandBuffer& commandBuffer, uint32_t imageIndex,
                                       std::vector<GameObject*>& gameObjects, SwapChain& swapChain,
                                               PipelineHandler& pipelineHandler, uint32_t currentFrame,
                                               AssetManager& assetManager, std::vector<MeshInfoComponent*>& meshInfo)
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

	commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
	                                          static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
	commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

	for (int i = 0; i < gameObjects.size(); i++)
	{
		// Bind vertex and index buffers
		commandBuffer.bindVertexBuffers(0, *assetManager.meshes[meshInfo[i]->bufferIndex].vertexBuffer, {0});
		commandBuffer.bindIndexBuffer(*assetManager.meshes[meshInfo[i]->bufferIndex].indexBuffer, 0,
		                              vk::IndexType::eUint32);
		// Bind the descriptor set for this object
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineHandler.pipelineLayout, 0,
		                                 *gameObjects[i]->descriptorSets[currentFrame], nullptr);
		commandBuffer.drawIndexed(meshInfo[i]->indexCount, 1, meshInfo[i]->indexOffset, meshInfo[i]->vertexOffset, 0);
	}
	commandBuffer.endRendering();

	transitionImageLayout(commandBuffer, swapChain.swapChainImages[imageIndex], vk::ImageLayout::eColorAttachmentOptimal,
	                      vk::ImageLayout::ePresentSrcKHR, vk::AccessFlagBits2::eColorAttachmentWrite, {},
	                      vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eBottomOfPipe,
	                      vk::ImageAspectFlagBits::eColor);

	commandBuffer.end();
}

void CommandBufferFactory::transitionImageLayout(vk::raii::CommandBuffer& commandBuffer, vk::Image image,
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