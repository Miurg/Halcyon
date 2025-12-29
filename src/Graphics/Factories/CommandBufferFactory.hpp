#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "../SwapChain.hpp"
#include "../PipelineHandler.hpp"
#include "../GameObject.hpp"
#include "../Resources/Managers/VertexIndexBuffer.hpp"
#include "../Resources/Managers/AssetManager.hpp"

class CommandBufferFactory
{
public:
	static void recordCommandBuffer(vk::raii::CommandBuffer& commandBuffer, uint32_t imageIndex,
	                                              std::vector<GameObject*>& gameObjects, SwapChain& swapChain,
	                                PipelineHandler& pipelineHandler, uint32_t currentFrame, AssetManager& assetManager,
	                                std::vector<MeshInfoComponent*>& meshInfo);
	static void transitionImageLayout(vk::raii::CommandBuffer& commandBuffer, vk::Image image, vk::ImageLayout oldLayout,
	                                  vk::ImageLayout newLayout, vk::AccessFlags2 srcAccessMask,
	                                  vk::AccessFlags2 dstAccessMask, vk::PipelineStageFlags2 srcStageMask,
	                                  vk::PipelineStageFlags2 dstStageMask, vk::ImageAspectFlags imageAspectFlags);
};
