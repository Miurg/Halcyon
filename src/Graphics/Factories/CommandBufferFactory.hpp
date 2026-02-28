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
#include "../Components/DrawInfoComponent.hpp"
#include "../Components/SsaoSettingsComponent.hpp"

// Records secondary command buffers for each render pass (shadow, cull, main, FXAA).
class CommandBufferFactory
{
public:
	static void recordShadowCommandBuffer(vk::raii::CommandBuffer& secondaryCmd, PipelineHandler& pipelineHandler,
	                                      uint32_t currentFrame, LightComponent& lightTexture,
	                                      DescriptorManagerComponent& dManager, GlobalDSetComponent* globalDSetComponent,
	                                      ModelDSetComponent* objectDSetComponent, TextureManager& tManager,
	                                      ModelManager& mManager);
	static void recordCullCommandBuffer(vk::raii::CommandBuffer& secondaryCmd, PipelineHandler& pipelineHandler,
	                                    uint32_t currentFrame, DescriptorManagerComponent& dManager,
	                                    GlobalDSetComponent* globalDSetComponent,
	                                    ModelDSetComponent* objectDSetComponent, ModelManager& mManager,
	                                    const DrawInfoComponent& drawInfo);

	static void recordMainCommandBuffer(vk::raii::CommandBuffer& secondaryCmd, uint32_t imageIndex, SwapChain& swapChain,
	                                    PipelineHandler& pipelineHandler, uint32_t currentFrame,
	                                    BindlessTextureDSetComponent& bindlessTextureDSetComponent,
	                                    DescriptorManagerComponent& dManager, GlobalDSetComponent* globalDSetComponent,
	                                    BufferManager& bManager, ModelDSetComponent* objectDSetComponent,
	                                    ModelManager& mManager, TextureManager& tManager,
	                                    const DrawInfoComponent& drawInfo);
	static void recordFxaaCommandBuffer(vk::raii::CommandBuffer& secondaryCmd, uint32_t imageIndex, SwapChain& swapChain,
	                                    PipelineHandler& pipelineHandler, DescriptorManagerComponent& dManager,
	                                    DSetHandle fxaaDescriptorSetIndex);
	static void recordSsaoCommandBuffer(vk::raii::CommandBuffer& secondaryCmd, SwapChain& swapChain,
	                                    PipelineHandler& pipelineHandler, DescriptorManagerComponent& dManager,
	                                    DSetHandle ssaoDescriptorSetIndex, DSetHandle globalDescriptorSetIndex,
	                                    TextureManager& tManager, const SsaoSettingsComponent& ssaoSettings);
	static void recordSsaoBlurCommandBuffer(vk::raii::CommandBuffer& secondaryCmd, SwapChain& swapChain,
	                                        PipelineHandler& pipelineHandler, DescriptorManagerComponent& dManager,
	                                        DSetHandle ssaoBlurDescriptorSetIndex, TextureManager& tManager);
	static void executeSecondaryBuffers(vk::raii::CommandBuffer& primaryCommandBuffer,
	                                    const vk::raii::CommandBuffers& secondaryBuffers);
	static void transitionImageLayout(vk::raii::CommandBuffer& commandBuffer, vk::Image image, vk::ImageLayout oldLayout,
	                                  vk::ImageLayout newLayout, vk::AccessFlags2 srcAccessMask,
	                                  vk::AccessFlags2 dstAccessMask, vk::PipelineStageFlags2 srcStageMask,
	                                  vk::PipelineStageFlags2 dstStageMask, vk::ImageAspectFlags imageAspectFlags,
	                                  uint32_t layerCount = 1, uint32_t mipLevelCount = 1);
};

// Aggregate of references passed into primary command buffer recording.
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