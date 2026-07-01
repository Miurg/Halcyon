#pragma once

#include "HalcyonExport.hpp"
#include <vulkan/vulkan_raii.hpp>
#include <cstdint>

class BufferManager;
class ModelManager;
class TextureManager;
class PipelineManager;
struct DescriptorManagerComponent;
struct GlobalDSetComponent;
struct ModelDSetComponent;
struct DrawInfoComponent;
struct DirectLightComponent;
struct BindlessTextureDSetComponent;

HALCYON_API void drawResetInstancePass(vk::raii::CommandBuffer& cmd, uint32_t frame, DescriptorManagerComponent& dManager,
                           ModelDSetComponent& objectDSetComponent, const DrawInfoComponent& drawInfo,
                           PipelineManager& pManager);

HALCYON_API void drawCullPass(vk::raii::CommandBuffer& cmd, uint32_t frame, DescriptorManagerComponent& dManager,
                  GlobalDSetComponent& globalDSetComponent, ModelDSetComponent& objectDSetComponent,
                  ModelManager& mManager, BufferManager& bManager, const DrawInfoComponent& drawInfo,
                  PipelineManager& pManager);

HALCYON_API void drawShadowCullPass(vk::raii::CommandBuffer& cmd, uint32_t frame, DescriptorManagerComponent& dManager,
                        GlobalDSetComponent& globalDSetComponent, ModelDSetComponent& objectDSetComponent,
                        ModelManager& mManager, BufferManager& bManager, const DrawInfoComponent& drawInfo,
                        PipelineManager& pManager);

HALCYON_API void drawShadowPass(vk::raii::CommandBuffer& cmd, uint32_t frame, DirectLightComponent& lightTexture,
                    DescriptorManagerComponent& dManager, GlobalDSetComponent& globalDSetComponent,
                    ModelDSetComponent& objectDSetComponent, BindlessTextureDSetComponent& bTextureDSet,
                    TextureManager& tManager, ModelManager& mManager, BufferManager& bManager,
                    const DrawInfoComponent& drawInfo, PipelineManager& pManager);
