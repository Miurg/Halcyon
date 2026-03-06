#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "../SwapChain.hpp"
#include "../PipelineHandler.hpp"
#include "../Resources/Managers/BufferManager.hpp"
#include "../Components/LightComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Resources/Components/ModelDSetComponent.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"
#include "../Resources/Managers/TextureManager.hpp"
#include "../Resources/Managers/ModelManager.hpp"
#include "../Components/DrawInfoComponent.hpp"
#include "../Components/SsaoSettingsComponent.hpp"

class CommandBufferFactory
{
public:
	static void drawShadowCullPass(vk::raii::CommandBuffer& cmd, PipelineHandler& pipelineHandler, uint32_t currentFrame,
	                               DescriptorManagerComponent& dManager, GlobalDSetComponent* globalDSetComponent,
	                               ModelDSetComponent* objectDSetComponent, ModelManager& mManager,
	                               BufferManager& bManager, const DrawInfoComponent& drawInfo);
	static void drawShadowPass(vk::raii::CommandBuffer& cmd, PipelineHandler& pipelineHandler, uint32_t currentFrame,
	                           LightComponent& lightTexture, DescriptorManagerComponent& dManager,
	                           GlobalDSetComponent* globalDSetComponent, ModelDSetComponent* objectDSetComponent,
	                           TextureManager& tManager, ModelManager& mManager, BufferManager& bManager,
	                           const DrawInfoComponent& drawInfo);

	static void drawResetInstancePass(vk::raii::CommandBuffer& cmd, PipelineHandler& pipelineHandler,
	                                  uint32_t currentFrame, DescriptorManagerComponent& dManager,
	                                  ModelDSetComponent* objectDSetComponent, const DrawInfoComponent& drawInfo);

	static void drawCullPass(vk::raii::CommandBuffer& cmd, PipelineHandler& pipelineHandler, uint32_t currentFrame,
	                         DescriptorManagerComponent& dManager, GlobalDSetComponent* globalDSetComponent,
	                         ModelDSetComponent* objectDSetComponent, ModelManager& mManager, BufferManager& bManager,
	                         const DrawInfoComponent& drawInfo);

	static void drawDepthPrepass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, PipelineHandler& pipelineHandler,
	                             uint32_t currentFrame, BindlessTextureDSetComponent& bindlessTextureDSetComponent,
	                             DescriptorManagerComponent& dManager, GlobalDSetComponent* globalDSetComponent,
	                             BufferManager& bManager, ModelDSetComponent* objectDSetComponent,
	                             ModelManager& mManager, const DrawInfoComponent& drawInfo);

	static void drawMainPass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, PipelineHandler& pipelineHandler,
	                         uint32_t currentFrame, BindlessTextureDSetComponent& bindlessTextureDSetComponent,
	                         DescriptorManagerComponent& dManager, GlobalDSetComponent* globalDSetComponent,
	                         BufferManager& bManager, ModelDSetComponent* objectDSetComponent, ModelManager& mManager,
	                         const DrawInfoComponent& drawInfo);

	static void drawFxaaPass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, PipelineHandler& pipelineHandler,
	                         DescriptorManagerComponent& dManager, DSetHandle fxaaDescriptorSetIndex);

	static void drawSsaoPass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, PipelineHandler& pipelineHandler,
	                         DescriptorManagerComponent& dManager, DSetHandle ssaoDescriptorSetIndex,
	                         DSetHandle globalDescriptorSetIndex, const SsaoSettingsComponent& ssaoSettings);

	static void drawSsaoBlurPass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, PipelineHandler& pipelineHandler,
	                             DescriptorManagerComponent& dManager, DSetHandle ssaoBlurDescriptorSetIndex);

	static void transitionImageLayout(vk::raii::CommandBuffer& commandBuffer, vk::Image image, vk::ImageLayout oldLayout,
	                                  vk::ImageLayout newLayout, vk::AccessFlags2 srcAccessMask,
	                                  vk::AccessFlags2 dstAccessMask, vk::PipelineStageFlags2 srcStageMask,
	                                  vk::PipelineStageFlags2 dstStageMask, vk::ImageAspectFlags imageAspectFlags,
	                                  uint32_t layerCount = 1, uint32_t mipLevelCount = 1);
};