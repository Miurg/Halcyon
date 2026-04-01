#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "../SwapChain.hpp"
#include "../Resources/Managers/BufferManager.hpp"
#include "../Components/DirectLightComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Resources/Components/ModelDSetComponent.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"
#include "../Resources/Managers/TextureManager.hpp"
#include "../Resources/Managers/ModelManager.hpp"
#include "../Components/DrawInfoComponent.hpp"
#include "../Components/SsaoSettingsComponent.hpp"
#include "../Managers/PipelineManager.hpp"


class CommandBufferFactory
{
public:
	static void drawShadowCullPass(vk::raii::CommandBuffer& cmd, uint32_t currentFrame,
	                               DescriptorManagerComponent& dManager, GlobalDSetComponent* globalDSetComponent,
	                               ModelDSetComponent* objectDSetComponent, ModelManager& mManager,
	                               BufferManager& bManager, const DrawInfoComponent& drawInfo,
	                               PipelineManager& pManager);
	static void drawShadowPass(vk::raii::CommandBuffer& cmd, uint32_t currentFrame,
	                           DirectLightComponent& lightTexture, DescriptorManagerComponent& dManager,
	                           GlobalDSetComponent* globalDSetComponent, ModelDSetComponent* objectDSetComponent,
	                           TextureManager& tManager, ModelManager& mManager, BufferManager& bManager, const DrawInfoComponent& drawInfo, PipelineManager& pManager);

	static void drawResetInstancePass(vk::raii::CommandBuffer& cmd,
	                                  uint32_t currentFrame, DescriptorManagerComponent& dManager,
	                                  ModelDSetComponent* objectDSetComponent, const DrawInfoComponent& drawInfo,
	                                  PipelineManager& pManager);

	static void drawCullPass(vk::raii::CommandBuffer& cmd, uint32_t currentFrame,
	                         DescriptorManagerComponent& dManager, GlobalDSetComponent* globalDSetComponent,
	                         ModelDSetComponent* objectDSetComponent, ModelManager& mManager, BufferManager& bManager,
	                         const DrawInfoComponent& drawInfo, PipelineManager& pManager);

	static void drawDepthPrepass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain,
	                             uint32_t currentFrame, BindlessTextureDSetComponent& bindlessTextureDSetComponent,
	                             DescriptorManagerComponent& dManager, GlobalDSetComponent* globalDSetComponent,
	                             BufferManager& bManager, ModelDSetComponent* objectDSetComponent,
	                             ModelManager& mManager, const DrawInfoComponent& drawInfo, PipelineManager& pManager);

	static void drawMainPass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain,
	                         uint32_t currentFrame, BindlessTextureDSetComponent& bindlessTextureDSetComponent,
	                         DescriptorManagerComponent& dManager, GlobalDSetComponent* globalDSetComponent,
	                         BufferManager& bManager, ModelDSetComponent* objectDSetComponent, ModelManager& mManager,
	                         const DrawInfoComponent& drawInfo, PipelineManager& pManager);

	static void drawFxaaPass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain,
	                         DescriptorManagerComponent& dManager, DSetHandle fxaaDescriptorSetIndex, PipelineManager& pManager);

	static void drawSsaoPass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain,
	                         DescriptorManagerComponent& dManager, DSetHandle ssaoDescriptorSetIndex, DSetHandle globalDescriptorSetIndex,
	                         const SsaoSettingsComponent& ssaoSettings, uint32_t frameNumber, PipelineManager& pManager);

	static void drawSsaoBlurPass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain,
	                             DescriptorManagerComponent& dManager, DSetHandle ssaoBlurDescriptorSetIndex, float dirX,
	                             float dirY, PipelineManager& pManager);
	static void drawSSAOApplyPass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain,
	                              DescriptorManagerComponent& dManager, DSetHandle fxaaDescriptorSetIndex,
	                              PipelineManager& pManager);

	static void drawToneMappingPass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain,
	                                DescriptorManagerComponent& dManager, DSetHandle toneMappingDescriptorSetIndex,
	                                PipelineManager& pManager);

	static void drawVignettePass(vk::raii::CommandBuffer& cmd, SwapChain& swapChain, DescriptorManagerComponent& dManager,
	                      DSetHandle vignetteDescriptorSetIndex, PipelineManager& pManager);

	static void drawBloomDownsamplePass(vk::raii::CommandBuffer& cmd, 
	                                    DescriptorManagerComponent& dManager,
	                                    DSetHandle dSetHandle, PipelineManager& pManager,
	                                    float texelSizeX, float texelSizeY, float threshold, float knee,
	                                    int isFirstPass, vk::Extent2D extent);

	static void drawBloomUpsamplePass(vk::raii::CommandBuffer& cmd, DescriptorManagerComponent& dManager,
	                                  DSetHandle dSetHandle, PipelineManager& pManager,
	                                  float texelSizeX, float texelSizeY, float blendFactor,
	                                  int isLastPass, vk::Extent2D extent);


	static void drawImGui(vk::raii::CommandBuffer& cmd);

	static void transitionImageLayout(vk::raii::CommandBuffer& commandBuffer, vk::Image image, vk::ImageLayout oldLayout,
	                                  vk::ImageLayout newLayout, vk::AccessFlags2 srcAccessMask,
	                                  vk::AccessFlags2 dstAccessMask, vk::PipelineStageFlags2 srcStageMask,
	                                  vk::PipelineStageFlags2 dstStageMask, vk::ImageAspectFlags imageAspectFlags,
	                                  uint32_t layerCount = 1, uint32_t mipLevelCount = 1);
};