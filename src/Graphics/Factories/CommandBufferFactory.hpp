#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "../SwapChain.hpp"
#include "../PipelineHandler.hpp"
#include "../Resources/Managers/VertexIndexBuffer.hpp"
#include "../Resources/Managers/BufferManager.hpp"
#include "../Components/CameraComponent.hpp"
#include "../Components/LightComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Resources/Components/ModelDSetComponent.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"
#include "../Resources/Managers/TextureManager.hpp"
#include "../Resources/Managers/ModelManager.hpp"

class CommandBufferFactory
{
public:
	static void recordCommandBuffer(vk::raii::CommandBuffer& commandBuffer, uint32_t imageIndex, SwapChain& swapChain,
	                                PipelineHandler& pipelineHandler, uint32_t currentFrame,
	                                BufferManager& bufferManager, LightComponent& lightTexture,
	                                BindlessTextureDSetComponent& bindlessTextureDSetComponent,
	                                DescriptorManagerComponent& dManager, GlobalDSetComponent* globalDSetComponent,
	                                ModelDSetComponent* objectDSetComponent, TextureManager& tManager,
	                                ModelManager& mManager);
	static void transitionImageLayout(vk::raii::CommandBuffer& commandBuffer, vk::Image image, vk::ImageLayout oldLayout,
	                                  vk::ImageLayout newLayout, vk::AccessFlags2 srcAccessMask,
	                                  vk::AccessFlags2 dstAccessMask, vk::PipelineStageFlags2 srcStageMask,
	                                  vk::PipelineStageFlags2 dstStageMask, vk::ImageAspectFlags imageAspectFlags);
};

struct CommandBufferStruct
{
	vk::raii::CommandBuffer& commandBuffer;
	uint32_t imageIndex;
	std::vector<int>& textureInfo;
	SwapChain& swapChain;
	PipelineHandler& pipelineHandler;
	uint32_t currentFrame;
	BufferManager& bufferManager;
	std::vector<MeshInfoComponent*>& meshInfo;
	CameraComponent& camera;
	LightComponent& lightTexture;
	BindlessTextureDSetComponent& materialDSet;
	DescriptorManagerComponent& dManager;
	GlobalDSetComponent* globalDSetComponent;
	ModelDSetComponent* objectDSetComponent;
};