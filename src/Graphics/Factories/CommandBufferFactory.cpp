#include "CommandBufferFactory.hpp"
#include <imgui.h>
#include <imgui_impl_vulkan.h>

void CommandBufferFactory::drawShadowPass(vk::raii::CommandBuffer& cmd, PipelineHandler& pipelineHandler,
                                          uint32_t currentFrame, LightComponent& lightTexture,
                                          DescriptorManagerComponent& dManager,
                                          GlobalDSetComponent* globalDSetComponent,
                                          ModelDSetComponent* objectDSetComponent, TextureManager& tManager,
                                          ModelManager& mManager)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineHandler.shadowPipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineHandler.pipelineLayout, 0,
	                       dManager.descriptorManager->descriptorSets[globalDSetComponent->globalDSets.id][currentFrame],
	                       nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *pipelineHandler.pipelineLayout, 1,
	    dManager.descriptorManager->descriptorSets[objectDSetComponent->modelBufferDSet.id][currentFrame], nullptr);
	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, lightTexture.sizeX, lightTexture.sizeY, 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(lightTexture.sizeX, lightTexture.sizeY)));

	uint32_t currentInstanceOffset = 0;
	uint32_t currentBuffer = -1;
	for (int i = 0; i < mManager.meshes.size(); i++)
	{
		if (mManager.meshes[i].vertexIndexBufferID != currentBuffer)
		{
			currentBuffer = mManager.meshes[i].vertexIndexBufferID;
			cmd.bindVertexBuffers(0, mManager.vertexIndexBuffers[mManager.meshes[i].vertexIndexBufferID].vertexBuffer,
			                      {0});
			cmd.bindIndexBuffer(mManager.vertexIndexBuffers[mManager.meshes[i].vertexIndexBufferID].indexBuffer, 0,
			                    vk::IndexType::eUint32);
		}
		for (int j = 0; j < mManager.meshes[i].primitives.size(); j++)
		{
			cmd.drawIndexed(mManager.meshes[i].primitives[j].indexCount, mManager.meshes[i].entitiesSubscribed,
			                mManager.meshes[i].primitives[j].indexOffset, mManager.meshes[i].primitives[j].vertexOffset,
			                currentInstanceOffset);
			currentInstanceOffset += mManager.meshes[i].entitiesSubscribed;
		}
	}
}

void CommandBufferFactory::drawCullPass(vk::raii::CommandBuffer& cmd, PipelineHandler& pipelineHandler,
                                        uint32_t currentFrame, DescriptorManagerComponent& dManager,
                                        GlobalDSetComponent* globalDSetComponent,
                                        ModelDSetComponent* objectDSetComponent, ModelManager& mManager,
                                        const DrawInfoComponent& drawInfo)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pipelineHandler.cullingPipeline);

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pipelineHandler.cullingPipelineLayout, 0,
	                       dManager.descriptorManager->descriptorSets[globalDSetComponent->globalDSets.id][currentFrame],
	                       nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eCompute, *pipelineHandler.cullingPipelineLayout, 1,
	    dManager.descriptorManager->descriptorSets[objectDSetComponent->modelBufferDSet.id][currentFrame], nullptr);

	struct PushConsts
	{
		uint32_t objectCount;
	} push;

	push.objectCount = drawInfo.totalObjectCount;

	cmd.pushConstants<PushConsts>(*pipelineHandler.cullingPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, push);
	uint32_t groupCountX = (drawInfo.totalObjectCount + 63) / 64;
	if (groupCountX > 0) cmd.dispatch(groupCountX, 1, 1);

	vk::MemoryBarrier2 barrier;
	barrier.srcStageMask = vk::PipelineStageFlagBits2::eComputeShader;
	barrier.srcAccessMask = vk::AccessFlagBits2::eShaderWrite | vk::AccessFlagBits2::eShaderRead;
	barrier.dstStageMask = vk::PipelineStageFlagBits2::eDrawIndirect | vk::PipelineStageFlagBits2::eVertexShader;
	barrier.dstAccessMask = vk::AccessFlagBits2::eIndirectCommandRead | vk::AccessFlagBits2::eShaderRead;

	vk::DependencyInfo depInfo;
	depInfo.memoryBarrierCount = 1;
	depInfo.pMemoryBarriers = &barrier;

	cmd.pipelineBarrier2(depInfo);
}

void CommandBufferFactory::drawDepthPrepass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain,
                                            PipelineHandler& pipelineHandler, uint32_t currentFrame,
                                            BindlessTextureDSetComponent& bindlessTextureDSetComponent,
                                            DescriptorManagerComponent& dManager,
                                            GlobalDSetComponent* globalDSetComponent, BufferManager& bManager,
                                            ModelDSetComponent* objectDSetComponent, ModelManager& mManager,
                                            const DrawInfoComponent& drawInfo)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineHandler.depthPrepassPipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineHandler.pipelineLayout, 0,
	                       dManager.descriptorManager->descriptorSets[globalDSetComponent->globalDSets.id][currentFrame],
	                       nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *pipelineHandler.pipelineLayout, 1,
	    dManager.descriptorManager->descriptorSets[objectDSetComponent->modelBufferDSet.id][currentFrame], nullptr);

	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
	                                static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

	const uint32_t commandStride = sizeof(VkDrawIndexedIndirectCommand);
	cmd.bindVertexBuffers(0, mManager.vertexIndexBuffers[mManager.meshes[0].vertexIndexBufferID].vertexBuffer, {0});
	cmd.bindIndexBuffer(mManager.vertexIndexBuffers[mManager.meshes[0].vertexIndexBufferID].indexBuffer, 0,
	                    vk::IndexType::eUint32);
	uint32_t opaqueCount = drawInfo.opaqueDrawCount;
	if (opaqueCount > 0)
	{
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineHandler.depthPrepassPipeline);
		cmd.drawIndexedIndirect(bManager.buffers[objectDSetComponent->indirectDrawBuffer.id].buffer[currentFrame], 0,
		                        opaqueCount, commandStride);
	}
}

void CommandBufferFactory::drawMainPass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain,
                                        PipelineHandler& pipelineHandler, uint32_t currentFrame,
                                        BindlessTextureDSetComponent& bindlessTextureDSetComponent,
                                        DescriptorManagerComponent& dManager, GlobalDSetComponent* globalDSetComponent,
                                        BufferManager& bManager, ModelDSetComponent* objectDSetComponent,
                                        ModelManager& mManager, const DrawInfoComponent& drawInfo)
{
	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
	                                static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineHandler.pipelineLayout, 0,
	                       dManager.descriptorManager->descriptorSets[globalDSetComponent->globalDSets.id][currentFrame],
	                       nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *pipelineHandler.pipelineLayout, 1,
	    dManager.descriptorManager->descriptorSets[objectDSetComponent->modelBufferDSet.id][currentFrame], nullptr);
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics, *pipelineHandler.pipelineLayout, 2,
	    dManager.descriptorManager->descriptorSets[bindlessTextureDSetComponent.bindlessTextureSet.id][0], nullptr);

	const uint32_t commandStride = sizeof(VkDrawIndexedIndirectCommand);
	cmd.bindVertexBuffers(0, mManager.vertexIndexBuffers[mManager.meshes[0].vertexIndexBufferID].vertexBuffer, {0});
	cmd.bindIndexBuffer(mManager.vertexIndexBuffers[mManager.meshes[0].vertexIndexBufferID].indexBuffer, 0,
	                    vk::IndexType::eUint32);

	// Opaque pass
	uint32_t opaqueCount = drawInfo.opaqueDrawCount;
	if (opaqueCount > 0)
	{
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineHandler.graphicsPipeline);
		cmd.drawIndexedIndirect(bManager.buffers[objectDSetComponent->indirectDrawBuffer.id].buffer[currentFrame], 0,
		                        opaqueCount, commandStride);
	}

	// Alpha test pass
	uint32_t alphaTestCount = drawInfo.totalDrawCount - opaqueCount;
	if (alphaTestCount > 0)
	{
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineHandler.alphaTestPipeline);
		cmd.drawIndexedIndirect(bManager.buffers[objectDSetComponent->indirectDrawBuffer.id].buffer[currentFrame],
		                        opaqueCount * commandStride, alphaTestCount, commandStride);
	}

	// Skybox pass
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineHandler.skyboxPipeline);
	cmd.draw(3, 1, 0, 0);
}

void CommandBufferFactory::drawFxaaPass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain,
                                        PipelineHandler& pipelineHandler, DescriptorManagerComponent& dManager,
                                        DSetHandle fxaaDescriptorSetIndex)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineHandler.fxaaPipeline);

	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
	                                static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineHandler.fxaaPipelineLayout, 0,
	                       dManager.descriptorManager->descriptorSets[fxaaDescriptorSetIndex.id][0], nullptr);

	struct PushConstants
	{
		float rcpFrame[2];
	} push;
	push.rcpFrame[0] = 1.0f / static_cast<float>(swapChain.swapChainExtent.width);
	push.rcpFrame[1] = 1.0f / static_cast<float>(swapChain.swapChainExtent.height);

	cmd.pushConstants<PushConstants>(*pipelineHandler.fxaaPipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, push);

	cmd.draw(3, 1, 0, 0);

	// ImGui
	ImDrawData* draw_data = ImGui::GetDrawData();
	if (draw_data)
	{
		ImGui_ImplVulkan_RenderDrawData(draw_data, *cmd);
	}
}

void CommandBufferFactory::drawSsaoPass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain,
                                        PipelineHandler& pipelineHandler, DescriptorManagerComponent& dManager,
                                        DSetHandle ssaoDescriptorSetIndex, DSetHandle globalDescriptorSetIndex,
                                        const SsaoSettingsComponent& ssaoSettings)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineHandler.ssaoPipeline);

	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width / 2),
	                                static_cast<float>(swapChain.swapChainExtent.height / 2), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D{swapChain.swapChainExtent.width / 2,
	                                                              swapChain.swapChainExtent.height / 2}));

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineHandler.ssaoPipelineLayout, 0,
	                       dManager.descriptorManager->descriptorSets[ssaoDescriptorSetIndex.id][0], nullptr);

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineHandler.ssaoPipelineLayout, 1,
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

	cmd.pushConstants<SsaoPushConstants>(*pipelineHandler.ssaoPipelineLayout, vk::ShaderStageFlagBits::eFragment, 0,
	                                     push);

	cmd.draw(3, 1, 0, 0);
}

void CommandBufferFactory::drawSsaoBlurPass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain,
                                            PipelineHandler& pipelineHandler, DescriptorManagerComponent& dManager,
                                            DSetHandle ssaoBlurDescriptorSetIndex)
{
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelineHandler.ssaoBlurPipeline);

	cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width / 2),
	                                static_cast<float>(swapChain.swapChainExtent.height / 2), 0.0f, 1.0f));
	cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D{swapChain.swapChainExtent.width / 2,
	                                                              swapChain.swapChainExtent.height / 2}));

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineHandler.ssaoBlurPipelineLayout, 0,
	                       dManager.descriptorManager->descriptorSets[ssaoBlurDescriptorSetIndex.id][0], nullptr);

	struct BlurPushConstants
	{
		float texelSize[2];
	} push;
	push.texelSize[0] = 1.0f / static_cast<float>(swapChain.swapChainExtent.width / 2);
	push.texelSize[1] = 1.0f / static_cast<float>(swapChain.swapChainExtent.height / 2);

	cmd.pushConstants<BlurPushConstants>(*pipelineHandler.ssaoBlurPipelineLayout, vk::ShaderStageFlagBits::eFragment, 0,
	                                     push);

	cmd.draw(3, 1, 0, 0);
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