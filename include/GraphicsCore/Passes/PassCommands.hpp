#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"
#include <vulkan/vulkan_raii.hpp>
#include <cstdint>

class BufferManager;
class ModelManager;
class TextureManager;
class PipelineManager;
class DescriptorManager;
struct DescriptorManagerComponent;
struct GlobalDSetComponent;
struct ModelDSetComponent;
struct DrawInfoComponent;
struct DirectLightComponent;
struct BindlessTextureDSetComponent;

struct DrawCursor
{
	vk::Buffer commandBuffer;
	vk::Buffer countBuffer;
	uint32_t commandStride = sizeof(VkDrawIndexedIndirectCommand);
	uint32_t commandOffset = 0;
	uint32_t countOffset = 0;

	void draw(vk::raii::CommandBuffer& cmd, uint32_t maxCount, vk::CullModeFlagBits cull)
	{
		if (maxCount > 0)
		{
			cmd.setCullMode(cull);
			cmd.drawIndexedIndirectCount(commandBuffer, commandOffset, countBuffer, countOffset, maxCount, commandStride);
			commandOffset += maxCount * commandStride;
		}
		countOffset += sizeof(uint32_t);
	}

	void skip(uint32_t maxCount)
	{
		commandOffset += maxCount * commandStride;
		countOffset += sizeof(uint32_t);
	}
};

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

HALCYON_API void recordSHProjection(vk::raii::CommandBuffer& cmd, int cubemapResolution, int probeSlot,
                                    DescriptorManager& descriptorManager, BindlessTextureDSetComponent& dSetComponent,
                                    DSetHandle globalDSet, PipelineManager& pipelineManager);

HALCYON_API void drawShadowPass(vk::raii::CommandBuffer& cmd, uint32_t frame, DirectLightComponent& lightTexture,
                    DescriptorManagerComponent& descriptorManager, GlobalDSetComponent& globalDSetComponent,
                    ModelDSetComponent& objectDSetComponent, BindlessTextureDSetComponent& bTextureDSet,
                    TextureManager& textureManager, ModelManager& modelManager, BufferManager& bufferManager,
                    const DrawInfoComponent& drawInfo, PipelineManager& pipelineManager);
