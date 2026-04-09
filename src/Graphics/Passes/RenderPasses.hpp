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
#include <Orhescyon/Entitys/EntityManager.hpp>
#include <iostream>
#include "../GraphicsContexts.hpp"
#include <imgui.h>
#include "../Components/SwapChainComponent.hpp"
#include "../Components/BufferManagerComponent.hpp"
#include "../FrameData.hpp"
#include "../Components/FrameDataComponent.hpp"
#include "../Components/CurrentFrameComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Components/FrameImageComponent.hpp"
#include "../Components/TextureManagerComponent.hpp"
#include "../Resources/Managers/TextureManager.hpp"
#include "../Components/ModelManagerComponent.hpp"
#include "../Managers/FrameManager.hpp"
#include "../Components/FrameManagerComponent.hpp"
#include "../Components/DrawInfoComponent.hpp"
#include "../Components/SsaoSettingsComponent.hpp"
#include "../Components/RenderGraphComponent.hpp"
#include "../RenderGraph/RenderGraph.hpp"
#include "../Components/GraphicsSettingsComponent.hpp"
#include "../Resources/Managers/Bindings.hpp"
#include "../GraphicsInit/GraphicsPipelinesInit.hpp"
#include "../Managers/PipelineManager.hpp"
#include "../Components/PipelineManagerComponent.hpp"
#include "../Components/SkyboxComponent.hpp"
#include "../Resources/Components/MeshInfoComponent.hpp"
#include "../Components/RelationshipComponent.hpp"
#include "../Components/GlobalTransformComponent.hpp"
#include <functional>

// Free helpers used by both CullPass and LightProbeGISystem (probe baking).
// TODO: do something, seems inconsistent
void drawResetInstancePass(vk::raii::CommandBuffer& cmd, uint32_t frame, DescriptorManagerComponent& dManager,
                           ModelDSetComponent& objectDSetComponent, const DrawInfoComponent& drawInfo,
                           PipelineManager& pManager);

void drawCullPass(vk::raii::CommandBuffer& cmd, uint32_t frame, DescriptorManagerComponent& dManager,
                  GlobalDSetComponent& globalDSetComponent, ModelDSetComponent& objectDSetComponent,
                  ModelManager& mManager, BufferManager& bManager, const DrawInfoComponent& drawInfo,
                  PipelineManager& pManager);

void drawShadowCullPass(vk::raii::CommandBuffer& cmd, uint32_t currentFrame,
                        DescriptorManagerComponent& dManager, GlobalDSetComponent& globalDSetComponent,
                        ModelDSetComponent& objectDSetComponent, ModelManager& mManager,
                        BufferManager& bManager, const DrawInfoComponent& drawInfo, PipelineManager& pManager);

void drawShadowPass(vk::raii::CommandBuffer& cmd, uint32_t currentFrame,
                    DirectLightComponent& lightTexture, DescriptorManagerComponent& dManager,
                    GlobalDSetComponent& globalDSetComponent, ModelDSetComponent& objectDSetComponent,
                    TextureManager& tManager, ModelManager& mManager, BufferManager& bManager,
                    const DrawInfoComponent& drawInfo, PipelineManager& pManager);

class RenderPasses
{
public:
	static void MainPass(SwapChain& swapChain, uint32_t currentFrame,
	                     BindlessTextureDSetComponent& bindlessTextureDSetComponent,
	                     DescriptorManagerComponent& dManager, GlobalDSetComponent& globalDSetComponent,
	                     BufferManager& bManager, ModelDSetComponent& objectDSetComponent, ModelManager& mManager,
	                     const DrawInfoComponent& drawInfo, PipelineManager& pManager, bool hasSkybox,
	                     GraphicsSettingsComponent& graphicsSettings, RenderGraph& rg);
	static void DepthPrepass(SwapChain& swapChain, uint32_t currentFrame,
	                         BindlessTextureDSetComponent& bindlessTextureDSetComponent,
	                         DescriptorManagerComponent& dManager, GlobalDSetComponent& globalDSetComponent,
	                         BufferManager& bManager, ModelDSetComponent& objectDSetComponent, ModelManager& mManager,
	                         const DrawInfoComponent& drawInfo, PipelineManager& pManager,
	                         GraphicsSettingsComponent& graphicsSettings, RenderGraph& rg);
	static void DebugPass(SwapChain& swapChain, DescriptorManagerComponent& dManager,
	                      GlobalDSetComponent& globalDSetComponent, PipelineManager& pManager, uint32_t currentFrame,
	                      RenderGraph& rg, GeneralManager& gm, GraphicsSettingsComponent& graphicsSettings,
	                      ModelManager& mManager);
	static void SSAOPass(SwapChain& swapChain, CurrentFrameComponent& currentFrameComp,
	                     BindlessTextureDSetComponent& bindlessTextureDSetComponent,
	                     DescriptorManagerComponent& dManager, GlobalDSetComponent& globalDSetComponent,
	                     PipelineManager& pManager, RenderGraph& rg, SsaoSettingsComponent& ssaoSettings);
	static void BloomPass(SwapChain& swapChain, DescriptorManagerComponent& dManager,
	                      GlobalDSetComponent& globalDSetComponent, PipelineManager& pManager, RenderGraph& rg,
	                      GraphicsSettingsComponent& graphicsSettings);
	static void ToneMappingPass(SwapChain& swapChain, DescriptorManagerComponent& dManager,
	                            GlobalDSetComponent& globalDSetComponent, PipelineManager& pManager, RenderGraph& rg);
	static void FXAAPass(SwapChain& swapChain, DescriptorManagerComponent& dManager,
	                     GlobalDSetComponent& globalDSetComponent, PipelineManager& pManager, RenderGraph& rg);
	static void VignettePass(SwapChain& swapChain, DescriptorManagerComponent& dManager,
	                         GlobalDSetComponent& globalDSetComponent, PipelineManager& pManager, RenderGraph& rg);
	static void DirectLightPass(uint32_t currentFrame, DescriptorManagerComponent& dManager,
	                            GlobalDSetComponent& globalDSetComponent, BufferManager& bManager,
	                            ModelDSetComponent& objectDSetComponent, ModelManager& mManager,
	                            const DrawInfoComponent& drawInfo, PipelineManager& pManager, RenderGraph& rg,
	                            TextureManager& textureManager, DirectLightComponent& lightTexture);
	static void CullPass(uint32_t currentFrame, DescriptorManagerComponent& dManager,
	                     GlobalDSetComponent& globalDSetComponent, BufferManager& bManager,
	                     ModelDSetComponent& objectDSetComponent, ModelManager& mManager,
	                     const DrawInfoComponent& drawInfo, PipelineManager& pManager, RenderGraph& rg);
};