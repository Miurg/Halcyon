#include "CommandBufferFactory.hpp"

void CommandBufferFactory::recordCommandBuffer(
    vk::raii::CommandBuffer& commandBuffer, uint32_t imageIndex, SwapChain& swapChain, PipelineHandler& pipelineHandler,
    uint32_t currentFrame, BufferManager& bufferManager, LightComponent& lightTexture,
    BindlessTextureDSetComponent& bindlessTextureDSetComponent, DescriptorManagerComponent& dManager,
    GlobalDSetComponent* globalDSetComponent, ModelDSetComponent* objectDSetComponent)
{
	commandBuffer.begin({});

	// Step 1: SHADOW PASS

	transitionImageLayout(commandBuffer, bufferManager.textures[lightTexture.textureShadowImage].textureImage,
	                      vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal,
	                      vk::AccessFlagBits2::eNone, vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
	                      vk::PipelineStageFlagBits2::eTopOfPipe, vk::PipelineStageFlagBits2::eEarlyFragmentTests,
	                      vk::ImageAspectFlagBits::eDepth);

	vk::RenderingAttachmentInfo shadowDepthInfo;
	shadowDepthInfo.imageView = bufferManager.textures[lightTexture.textureShadowImage].textureImageView;
	shadowDepthInfo.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
	shadowDepthInfo.loadOp = vk::AttachmentLoadOp::eClear;
	shadowDepthInfo.storeOp = vk::AttachmentStoreOp::eStore;
	shadowDepthInfo.clearValue = vk::ClearDepthStencilValue(1.0f, 0);

	vk::RenderingInfo shadowRenderingInfo;
	shadowRenderingInfo.renderArea.extent = vk::Extent2D(lightTexture.sizeX, lightTexture.sizeY);
	shadowRenderingInfo.layerCount = 1;
	shadowRenderingInfo.colorAttachmentCount = 0;
	shadowRenderingInfo.pDepthAttachment = &shadowDepthInfo;

	commandBuffer.beginRendering(shadowRenderingInfo);
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineHandler.shadowPipeline);
	commandBuffer.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *pipelineHandler.pipelineLayout, 0,
	    dManager.descriptorManager->descriptorSets[globalDSetComponent->globalDSets][currentFrame], nullptr);
	commandBuffer.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *pipelineHandler.pipelineLayout, 2,
	    dManager.descriptorManager->descriptorSets[objectDSetComponent->modelBufferDSet][currentFrame], nullptr);
	commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, lightTexture.sizeX, lightTexture.sizeY, 0.0f, 1.0f));
	commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(lightTexture.sizeX, lightTexture.sizeY)));

	uint32_t currentInstanceOffset = 0;
	for (int i = 0; i < bufferManager.meshes.size(); i++)
	{
		commandBuffer.bindVertexBuffers(
		    0, bufferManager.vertexIndexBuffers[bufferManager.meshes[i].vertexIndexBufferID].vertexBuffer,
		    {0});
		commandBuffer.bindIndexBuffer(
		    bufferManager.vertexIndexBuffers[bufferManager.meshes[i].vertexIndexBufferID].indexBuffer, 0,
		    vk::IndexType::eUint32);
		for (int j = 0; j < bufferManager.meshes[i].primitives.size(); j++)
		{
			commandBuffer.drawIndexed(bufferManager.meshes[i].primitives[j].indexCount,
			                          bufferManager.meshes[i].entitiesSubscribed,
			                          bufferManager.meshes[i].primitives[j].indexOffset,
			                          bufferManager.meshes[i].primitives[j].vertexOffset, currentInstanceOffset);
			currentInstanceOffset += bufferManager.meshes[i].entitiesSubscribed;
		}
	}
	commandBuffer.endRendering();

	transitionImageLayout(commandBuffer, bufferManager.textures[lightTexture.textureShadowImage].textureImage,
	                      vk::ImageLayout::eDepthAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
	                      vk::AccessFlagBits2::eDepthStencilAttachmentWrite, vk::AccessFlagBits2::eShaderRead,
	                      vk::PipelineStageFlagBits2::eLateFragmentTests, vk::PipelineStageFlagBits2::eFragmentShader,
	                      vk::ImageAspectFlagBits::eDepth);
	// Step 2: MAIN PASS

	transitionImageLayout(commandBuffer, swapChain.swapChainImages[imageIndex], vk::ImageLayout::eUndefined,
	                      vk::ImageLayout::eColorAttachmentOptimal, {}, vk::AccessFlagBits2::eColorAttachmentWrite,
	                      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
	                      vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::ImageAspectFlagBits::eColor);

	transitionImageLayout(commandBuffer, swapChain.depthImage, vk::ImageLayout::eUndefined,
	                      vk::ImageLayout::eDepthAttachmentOptimal, {},
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

	commandBuffer.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *pipelineHandler.pipelineLayout, 0,
	    dManager.descriptorManager->descriptorSets[globalDSetComponent->globalDSets][currentFrame], nullptr);
	commandBuffer.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *pipelineHandler.pipelineLayout, 2,
	    dManager.descriptorManager->descriptorSets[objectDSetComponent->modelBufferDSet][currentFrame], nullptr);
	commandBuffer.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *pipelineHandler.pipelineLayout, 1,
	    dManager.descriptorManager->descriptorSets[bindlessTextureDSetComponent.bindlessTextureSet][0], nullptr);

	currentInstanceOffset = 0;
	for (int i = 0; i < bufferManager.meshes.size(); i++)
	{
		commandBuffer.bindVertexBuffers(
		    0, bufferManager.vertexIndexBuffers[bufferManager.meshes[i].vertexIndexBufferID].vertexBuffer,
		    {0});
		commandBuffer.bindIndexBuffer(
		    bufferManager.vertexIndexBuffers[bufferManager.meshes[i].vertexIndexBufferID].indexBuffer, 0,
		    vk::IndexType::eUint32);
		for (int j = 0; j < bufferManager.meshes[i].primitives.size(); j++)
		{
			commandBuffer.drawIndexed(bufferManager.meshes[i].primitives[j].indexCount,
			                          bufferManager.meshes[i].entitiesSubscribed,
			                          bufferManager.meshes[i].primitives[j].indexOffset,
			                          bufferManager.meshes[i].primitives[j].vertexOffset, currentInstanceOffset);
			currentInstanceOffset += bufferManager.meshes[i].entitiesSubscribed;
		}

		
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
                                                 vk::PipelineStageFlags2 srcStageMask,
                                                 vk::PipelineStageFlags2 dstStageMask,
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