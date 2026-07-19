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

HALCYON_API void drawResetInstancePass(vk::raii::CommandBuffer& cmd, uint32_t frame, DescriptorManagerComponent& descriptorManager,
                           ModelDSetComponent& objectDSetComponent, const DrawInfoComponent& drawInfo,
                           PipelineManager& pipelineManager);

HALCYON_API void drawCullPass(vk::raii::CommandBuffer& cmd, uint32_t frame, DescriptorManagerComponent& descriptorManager,
                  GlobalDSetComponent& globalDSetComponent, ModelDSetComponent& objectDSetComponent,
                  ModelManager& modelManager, BufferManager& bufferManager, const DrawInfoComponent& drawInfo,
                  PipelineManager& pipelineManager);

HALCYON_API void drawShadowCullPass(vk::raii::CommandBuffer& cmd, uint32_t frame, DescriptorManagerComponent& descriptorManager,
                        GlobalDSetComponent& globalDSetComponent, ModelDSetComponent& objectDSetComponent,
                        ModelManager& modelManager, BufferManager& bufferManager, const DrawInfoComponent& drawInfo,
                        PipelineManager& pipelineManager);

HALCYON_API void drawShadowPass(vk::raii::CommandBuffer& cmd, uint32_t frame, DirectLightComponent& lightTexture,
                    DescriptorManagerComponent& descriptorManager, GlobalDSetComponent& globalDSetComponent,
                    ModelDSetComponent& objectDSetComponent, BindlessTextureDSetComponent& bTextureDSet,
                    TextureManager& textureManager, ModelManager& modelManager, BufferManager& bufferManager,
                    const DrawInfoComponent& drawInfo, PipelineManager& pipelineManager);
