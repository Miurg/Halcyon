#include "CommandBufferFactory.hpp"
#include <imgui.h>
#include <imgui_impl_vulkan.h>

void CommandBufferFactory::recordShadowCommandBuffer(vk::raii::CommandBuffer& secondaryCmd,
                                                     PipelineHandler& pipelineHandler, uint32_t currentFrame,
                                                     LightComponent& lightTexture, DescriptorManagerComponent& dManager,
                                                     GlobalDSetComponent* globalDSetComponent,
                                                     ModelDSetComponent* objectDSetComponent, TextureManager& tManager,
                                                     ModelManager& mManager)
{
	vk::CommandBufferInheritanceInfo inheritanceInfo = {};
	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	beginInfo.pInheritanceInfo = &inheritanceInfo;

	secondaryCmd.begin(beginInfo);

	// --- Step 1: SHADOW PASS ---

	vk::RenderingAttachmentInfo shadowDepthInfo;
	shadowDepthInfo.imageView = tManager.textures[lightTexture.textureShadowImage.id].textureImageView;
	shadowDepthInfo.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
	shadowDepthInfo.loadOp = vk::AttachmentLoadOp::eClear;
	shadowDepthInfo.storeOp = vk::AttachmentStoreOp::eStore;
	shadowDepthInfo.clearValue = vk::ClearDepthStencilValue(0.0f, 0);

	vk::RenderingInfo shadowRenderingInfo;
	shadowRenderingInfo.renderArea.extent = vk::Extent2D(lightTexture.sizeX, lightTexture.sizeY);
	shadowRenderingInfo.layerCount = 1;
	shadowRenderingInfo.colorAttachmentCount = 0;
	shadowRenderingInfo.pDepthAttachment = &shadowDepthInfo;

	secondaryCmd.beginRendering(shadowRenderingInfo);
	secondaryCmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineHandler.shadowPipeline);
	secondaryCmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *pipelineHandler.pipelineLayout, 0,
	    dManager.descriptorManager->descriptorSets[globalDSetComponent->globalDSets.id][currentFrame], nullptr);
	secondaryCmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *pipelineHandler.pipelineLayout, 1,
	    dManager.descriptorManager->descriptorSets[objectDSetComponent->modelBufferDSet.id][currentFrame], nullptr);
	secondaryCmd.setViewport(0, vk::Viewport(0.0f, 0.0f, lightTexture.sizeX, lightTexture.sizeY, 0.0f, 1.0f));
	secondaryCmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(lightTexture.sizeX, lightTexture.sizeY)));

	uint32_t currentInstanceOffset = 0;
	uint32_t currentBuffer = -1;
	for (int i = 0; i < mManager.meshes.size(); i++)
	{
		if (mManager.meshes[i].vertexIndexBufferID != currentBuffer)
		{
			currentBuffer = mManager.meshes[i].vertexIndexBufferID;
			secondaryCmd.bindVertexBuffers(
			    0, mManager.vertexIndexBuffers[mManager.meshes[i].vertexIndexBufferID].vertexBuffer, {0});
			secondaryCmd.bindIndexBuffer(mManager.vertexIndexBuffers[mManager.meshes[i].vertexIndexBufferID].indexBuffer,
			                             0, vk::IndexType::eUint32);
		}
		for (int j = 0; j < mManager.meshes[i].primitives.size(); j++)
		{
			secondaryCmd.drawIndexed(mManager.meshes[i].primitives[j].indexCount, mManager.meshes[i].entitiesSubscribed,
			                         mManager.meshes[i].primitives[j].indexOffset,
			                         mManager.meshes[i].primitives[j].vertexOffset, currentInstanceOffset);
			currentInstanceOffset += mManager.meshes[i].entitiesSubscribed;
		}
	}
	secondaryCmd.endRendering();

	secondaryCmd.end();
}

void CommandBufferFactory::recordCullCommandBuffer(vk::raii::CommandBuffer& secondaryCmd,
                                                   PipelineHandler& pipelineHandler, uint32_t currentFrame,
                                                   DescriptorManagerComponent& dManager,
                                                   GlobalDSetComponent* globalDSetComponent,
                                                   ModelDSetComponent* objectDSetComponent, ModelManager& mManager,
                                                   const DrawInfoComponent& drawInfo)
{
	// --- Step 1.5: CULLING PASS ---
	vk::CommandBufferInheritanceInfo inheritanceInfo = {};
	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	beginInfo.pInheritanceInfo = &inheritanceInfo;

	secondaryCmd.begin(beginInfo);

	secondaryCmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pipelineHandler.cullingPipeline);

	secondaryCmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eCompute, *pipelineHandler.cullingPipelineLayout, 0,
	    dManager.descriptorManager->descriptorSets[globalDSetComponent->globalDSets.id][currentFrame], nullptr);
	secondaryCmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eCompute, *pipelineHandler.cullingPipelineLayout, 1,
	    dManager.descriptorManager->descriptorSets[objectDSetComponent->modelBufferDSet.id][currentFrame], nullptr);

	struct PushConsts
	{
		uint32_t objectCount;
	} push;

	push.objectCount = drawInfo.totalObjectCount;

	secondaryCmd.pushConstants<PushConsts>(*pipelineHandler.cullingPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0,
	                                       push);
	uint32_t groupCountX = (drawInfo.totalObjectCount + 63) / 64;
	if (groupCountX > 0) secondaryCmd.dispatch(groupCountX, 1, 1);

	vk::MemoryBarrier2 barrier;
	barrier.srcStageMask = vk::PipelineStageFlagBits2::eComputeShader;
	barrier.srcAccessMask = vk::AccessFlagBits2::eShaderWrite | vk::AccessFlagBits2::eShaderRead;
	barrier.dstStageMask = vk::PipelineStageFlagBits2::eDrawIndirect | vk::PipelineStageFlagBits2::eVertexShader;
	barrier.dstAccessMask = vk::AccessFlagBits2::eIndirectCommandRead | vk::AccessFlagBits2::eShaderRead;

	vk::DependencyInfo depInfo;
	depInfo.memoryBarrierCount = 1;
	depInfo.pMemoryBarriers = &barrier;

	secondaryCmd.pipelineBarrier2(depInfo);

	secondaryCmd.end();
}

void CommandBufferFactory::recordDepthPrepassCommandBuffer(
    vk::raii::CommandBuffer& secondaryCmd, uint32_t imageIndex, SwapChain& swapChain, PipelineHandler& pipelineHandler,
    uint32_t currentFrame, BindlessTextureDSetComponent& bindlessTextureDSetComponent,
    DescriptorManagerComponent& dManager, GlobalDSetComponent* globalDSetComponent, BufferManager& bManager,
    ModelDSetComponent* objectDSetComponent, ModelManager& mManager, vk::ImageView depthImageView,
    const DrawInfoComponent& drawInfo)
{
	vk::CommandBufferInheritanceInfo inheritanceInfo = {};
	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	beginInfo.pInheritanceInfo = &inheritanceInfo;

	secondaryCmd.begin(beginInfo);

	// --- Step 1.9: DEPTH PREPASS ---

	vk::RenderingAttachmentInfo depthAttachmentInfo;
	depthAttachmentInfo.imageView = depthImageView;
	depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
	depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
	depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
	depthAttachmentInfo.clearValue = vk::ClearDepthStencilValue(1.0f, 0);

	vk::RenderingInfo renderingInfo;
	renderingInfo.renderArea.offset = 0;
	renderingInfo.layerCount = 1;
	renderingInfo.pDepthAttachment = &depthAttachmentInfo;
	renderingInfo.renderArea.extent = swapChain.swapChainExtent;

	secondaryCmd.beginRendering(renderingInfo);
	secondaryCmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineHandler.depthPrepassPipeline);
	secondaryCmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *pipelineHandler.pipelineLayout, 0,
	    dManager.descriptorManager->descriptorSets[globalDSetComponent->globalDSets.id][currentFrame], nullptr);
	secondaryCmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *pipelineHandler.pipelineLayout, 1,
	    dManager.descriptorManager->descriptorSets[objectDSetComponent->modelBufferDSet.id][currentFrame], nullptr);

	secondaryCmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
	                                         static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
	secondaryCmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

	const uint32_t commandStride = sizeof(VkDrawIndexedIndirectCommand);
	secondaryCmd.bindVertexBuffers(0, mManager.vertexIndexBuffers[mManager.meshes[0].vertexIndexBufferID].vertexBuffer,
	                               {0});
	secondaryCmd.bindIndexBuffer(mManager.vertexIndexBuffers[mManager.meshes[0].vertexIndexBufferID].indexBuffer, 0,
	                             vk::IndexType::eUint32);
	uint32_t opaqueCount = drawInfo.opaqueDrawCount;
	if (opaqueCount > 0)
	{
		secondaryCmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineHandler.depthPrepassPipeline);
		secondaryCmd.drawIndexedIndirect(
		    bManager.buffers[objectDSetComponent->indirectDrawBuffer.id].buffer[currentFrame], 0, opaqueCount,
		    commandStride);
	}

	secondaryCmd.endRendering();

	secondaryCmd.end();
}

void CommandBufferFactory::recordMainCommandBuffer(
    vk::raii::CommandBuffer& secondaryCmd, uint32_t imageIndex, SwapChain& swapChain, PipelineHandler& pipelineHandler,
    uint32_t currentFrame, BindlessTextureDSetComponent& bindlessTextureDSetComponent,
    DescriptorManagerComponent& dManager, GlobalDSetComponent* globalDSetComponent, BufferManager& bManager,
    ModelDSetComponent* objectDSetComponent, ModelManager& mManager, vk::ImageView offscreenImageView,
    vk::ImageView viewNormalsImageView, vk::ImageView depthImageView, const DrawInfoComponent& drawInfo)
{
	vk::CommandBufferInheritanceInfo inheritanceInfo = {};

	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	beginInfo.pInheritanceInfo = &inheritanceInfo;

	secondaryCmd.begin(beginInfo);

	// --- Step 2: MAIN PASS ---

	vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.637f, 1.0f, 1.0f);
	vk::RenderingAttachmentInfo attachmentInfo;
	attachmentInfo.imageView = offscreenImageView;
	attachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	attachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
	attachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
	attachmentInfo.clearValue = clearColor;

	vk::RenderingAttachmentInfo normalsAttachmentInfo;
	normalsAttachmentInfo.imageView = viewNormalsImageView;
	normalsAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	normalsAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
	normalsAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
	normalsAttachmentInfo.clearValue = vk::ClearValue(vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f));

	vk::RenderingAttachmentInfo depthAttachmentInfo;
	depthAttachmentInfo.imageView = depthImageView;
	depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
	depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eLoad;
	depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
	depthAttachmentInfo.clearValue = vk::ClearDepthStencilValue(1.0f, 0);

	std::array<vk::RenderingAttachmentInfo, 2> colorAttachments = {attachmentInfo, normalsAttachmentInfo};

	vk::RenderingInfo renderingInfo;
	renderingInfo.renderArea.offset = 0;
	renderingInfo.renderArea.extent = swapChain.swapChainExtent;
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
	renderingInfo.pColorAttachments = colorAttachments.data();
	renderingInfo.pDepthAttachment = &depthAttachmentInfo;

	secondaryCmd.beginRendering(renderingInfo);

	secondaryCmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
	                                         static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
	secondaryCmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

	secondaryCmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *pipelineHandler.pipelineLayout, 0,
	    dManager.descriptorManager->descriptorSets[globalDSetComponent->globalDSets.id][currentFrame], nullptr);
	secondaryCmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *pipelineHandler.pipelineLayout, 1,
	    dManager.descriptorManager->descriptorSets[objectDSetComponent->modelBufferDSet.id][currentFrame], nullptr);
	secondaryCmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *pipelineHandler.pipelineLayout, 2,
	    dManager.descriptorManager->descriptorSets[bindlessTextureDSetComponent.bindlessTextureSet.id][0], nullptr);

	uint32_t currentBuffer = -1;
	const uint32_t commandStride = sizeof(VkDrawIndexedIndirectCommand);
	secondaryCmd.bindVertexBuffers(0, mManager.vertexIndexBuffers[mManager.meshes[0].vertexIndexBufferID].vertexBuffer,
	                               {0});
	secondaryCmd.bindIndexBuffer(mManager.vertexIndexBuffers[mManager.meshes[0].vertexIndexBufferID].indexBuffer, 0,
	                             vk::IndexType::eUint32);

	// Opaque pass
	uint32_t opaqueCount = drawInfo.opaqueDrawCount;
	if (opaqueCount > 0)
	{
		secondaryCmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineHandler.graphicsPipeline);
		secondaryCmd.drawIndexedIndirect(
		    bManager.buffers[objectDSetComponent->indirectDrawBuffer.id].buffer[currentFrame], 0, opaqueCount,
		    commandStride);
	}

	// Alpha test pass
	uint32_t alphaTestCount = drawInfo.totalDrawCount - opaqueCount;
	if (alphaTestCount > 0)
	{
		secondaryCmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineHandler.alphaTestPipeline);
		secondaryCmd.drawIndexedIndirect(
		    bManager.buffers[objectDSetComponent->indirectDrawBuffer.id].buffer[currentFrame],
		    opaqueCount * commandStride, alphaTestCount, commandStride);
	}

	// Skybox pass
	secondaryCmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineHandler.skyboxPipeline);
	secondaryCmd.draw(3, 1, 0, 0);

	secondaryCmd.endRendering();

	secondaryCmd.end();
}

void CommandBufferFactory::recordFxaaCommandBuffer(vk::raii::CommandBuffer& secondaryCmd, uint32_t imageIndex,
                                                   SwapChain& swapChain, PipelineHandler& pipelineHandler,
                                                   DescriptorManagerComponent& dManager,
                                                   DSetHandle fxaaDescriptorSetIndex)
{
	vk::CommandBufferInheritanceInfo inheritanceInfo = {};
	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	beginInfo.pInheritanceInfo = &inheritanceInfo;

	secondaryCmd.begin(beginInfo);

	// --- Step 3: FXAA PASS ---

	vk::RenderingAttachmentInfo attachmentInfo;
	attachmentInfo.imageView = swapChain.swapChainImageViews[imageIndex];
	attachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	attachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
	attachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
	attachmentInfo.clearValue = vk::ClearValue(vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f));

	vk::RenderingInfo renderingInfo;
	renderingInfo.renderArea.offset = 0;
	renderingInfo.renderArea.extent = swapChain.swapChainExtent;
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &attachmentInfo;

	secondaryCmd.beginRendering(renderingInfo);

	secondaryCmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineHandler.fxaaPipeline);

	secondaryCmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
	                                         static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
	secondaryCmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

	secondaryCmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineHandler.fxaaPipelineLayout, 0,
	                                dManager.descriptorManager->descriptorSets[fxaaDescriptorSetIndex.id][0], nullptr);

	struct PushConstants
	{
		float rcpFrame[2];
	} push;
	push.rcpFrame[0] = 1.0f / static_cast<float>(swapChain.swapChainExtent.width);
	push.rcpFrame[1] = 1.0f / static_cast<float>(swapChain.swapChainExtent.height);

	secondaryCmd.pushConstants<PushConstants>(*pipelineHandler.fxaaPipelineLayout, vk::ShaderStageFlagBits::eFragment, 0,
	                                          push);

	// Draw full-screen triangle
	secondaryCmd.draw(3, 1, 0, 0);

	// --- Render ImGui ---
	ImDrawData* draw_data = ImGui::GetDrawData();
	if (draw_data)
	{
		ImGui_ImplVulkan_RenderDrawData(draw_data, *secondaryCmd);
	}

	secondaryCmd.endRendering();

	secondaryCmd.end();
}

void CommandBufferFactory::recordSsaoCommandBuffer(vk::raii::CommandBuffer& secondaryCmd, SwapChain& swapChain,
                                                   PipelineHandler& pipelineHandler,
                                                   DescriptorManagerComponent& dManager,
                                                   DSetHandle ssaoDescriptorSetIndex,
                                                   DSetHandle globalDescriptorSetIndex, vk::ImageView ssaoImageView,
                                                   const SsaoSettingsComponent& ssaoSettings)
{
	vk::CommandBufferInheritanceInfo inheritanceInfo = {};
	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	beginInfo.pInheritanceInfo = &inheritanceInfo;

	secondaryCmd.begin(beginInfo);

	vk::RenderingAttachmentInfo attachmentInfo;
	attachmentInfo.imageView = ssaoImageView;
	attachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	attachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
	attachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
	attachmentInfo.clearValue = vk::ClearValue(vk::ClearColorValue(1.0f, 1.0f, 1.0f, 1.0f));

	vk::RenderingInfo renderingInfo;
	renderingInfo.renderArea.offset = 0;
	renderingInfo.renderArea.extent =
	    vk::Extent2D{swapChain.swapChainExtent.width / 2, swapChain.swapChainExtent.height / 2};
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &attachmentInfo;

	secondaryCmd.beginRendering(renderingInfo);

	secondaryCmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineHandler.ssaoPipeline);

	secondaryCmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width / 2),
	                                         static_cast<float>(swapChain.swapChainExtent.height / 2), 0.0f, 1.0f));
	secondaryCmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D{swapChain.swapChainExtent.width / 2,
	                                                                       swapChain.swapChainExtent.height / 2}));

	secondaryCmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineHandler.ssaoPipelineLayout, 0,
	                                dManager.descriptorManager->descriptorSets[ssaoDescriptorSetIndex.id][0], nullptr);

	secondaryCmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineHandler.ssaoPipelineLayout, 1,
	                                dManager.descriptorManager->descriptorSets[globalDescriptorSetIndex.id][0], nullptr);

	struct SsaoPushConstants
	{
		int kernelSize;
		float radius;
		float bias;
		float power;
		int numDirections;
		float maxScreenRadius;
	} push;
	push.kernelSize = ssaoSettings.kernelSize;
	push.radius = ssaoSettings.radius;
	push.bias = ssaoSettings.bias;
	push.power = ssaoSettings.power;
	push.numDirections = ssaoSettings.numDirections;
	push.maxScreenRadius = ssaoSettings.maxScreenRadius;

	secondaryCmd.pushConstants<SsaoPushConstants>(*pipelineHandler.ssaoPipelineLayout,
	                                              vk::ShaderStageFlagBits::eFragment, 0, push);

	secondaryCmd.draw(3, 1, 0, 0);

	secondaryCmd.endRendering();

	secondaryCmd.end();
}

void CommandBufferFactory::recordSsaoBlurCommandBuffer(vk::raii::CommandBuffer& secondaryCmd, SwapChain& swapChain,
                                                       PipelineHandler& pipelineHandler,
                                                       DescriptorManagerComponent& dManager,
                                                       DSetHandle ssaoBlurDescriptorSetIndex,
                                                       vk::ImageView ssaoBlurImageView)
{
	vk::CommandBufferInheritanceInfo inheritanceInfo = {};
	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	beginInfo.pInheritanceInfo = &inheritanceInfo;

	secondaryCmd.begin(beginInfo);

	vk::RenderingAttachmentInfo attachmentInfo;
	attachmentInfo.imageView = ssaoBlurImageView;
	attachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	attachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
	attachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
	attachmentInfo.clearValue = vk::ClearValue(vk::ClearColorValue(1.0f, 1.0f, 1.0f, 1.0f));

	vk::RenderingInfo renderingInfo;
	renderingInfo.renderArea.offset = 0;
	renderingInfo.renderArea.extent =
	    vk::Extent2D{swapChain.swapChainExtent.width / 2, swapChain.swapChainExtent.height / 2};
	;
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &attachmentInfo;

	secondaryCmd.beginRendering(renderingInfo);

	secondaryCmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineHandler.ssaoBlurPipeline);

	secondaryCmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width / 2),
	                                         static_cast<float>(swapChain.swapChainExtent.height / 2), 0.0f, 1.0f));
	secondaryCmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D{swapChain.swapChainExtent.width / 2,
	                                                                       swapChain.swapChainExtent.height / 2}));

	secondaryCmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineHandler.ssaoBlurPipelineLayout, 0,
	                                dManager.descriptorManager->descriptorSets[ssaoBlurDescriptorSetIndex.id][0],
	                                nullptr);

	struct BlurPushConstants
	{
		float texelSize[2];
	} push;
	push.texelSize[0] = 1.0f / static_cast<float>(swapChain.swapChainExtent.width / 2);
	push.texelSize[1] = 1.0f / static_cast<float>(swapChain.swapChainExtent.height / 2);

	secondaryCmd.pushConstants<BlurPushConstants>(*pipelineHandler.ssaoBlurPipelineLayout,
	                                              vk::ShaderStageFlagBits::eFragment, 0, push);

	secondaryCmd.draw(3, 1, 0, 0);

	secondaryCmd.endRendering();

	secondaryCmd.end();
}

void CommandBufferFactory::executeSecondaryBuffers(vk::raii::CommandBuffer& primaryCommandBuffer,
                                                   const vk::raii::CommandBuffers& secondaryBuffers)
{
	primaryCommandBuffer.begin({});

	std::vector<vk::CommandBuffer> handles;
	handles.reserve(secondaryBuffers.size());

	for (const auto& cmd : secondaryBuffers)
	{
		handles.push_back(*cmd);
	}

	if (!handles.empty())
	{
		primaryCommandBuffer.executeCommands(handles);
	}

	primaryCommandBuffer.end();
}

void CommandBufferFactory::transitionImageLayout(vk::raii::CommandBuffer& commandBuffer, vk::Image image,
                                                 vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                                                 vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask,
                                                 vk::PipelineStageFlags2 srcStageMask,
                                                 vk::PipelineStageFlags2 dstStageMask,
                                                 vk::ImageAspectFlags imageAspectFlags, uint32_t layerCount,
                                                 uint32_t mipLevelCount)
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
	barrier.subresourceRange.levelCount = mipLevelCount;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = layerCount;

	vk::DependencyInfo dependencyInfo;
	dependencyInfo.dependencyFlags = {};
	dependencyInfo.imageMemoryBarrierCount = 1;
	dependencyInfo.pImageMemoryBarriers = &barrier;
	commandBuffer.pipelineBarrier2(dependencyInfo);
}