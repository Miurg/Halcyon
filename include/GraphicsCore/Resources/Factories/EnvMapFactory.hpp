#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"

class TextureManager;
class DescriptorManager;
class PipelineManager;
struct BindlessTextureDSetComponent;
struct VulkanDevice;

class HALCYON_API EnvMapFactory
{
public:
	static TextureHandle cubemapFromHdr(TextureManager& textureManager, VulkanDevice& vulkanDevice, TextureHandle hdrTexture,
	                                    DescriptorManager& descriptorManager, BindlessTextureDSetComponent& dSetComponent,
	                                    PipelineManager& pipelineManager);
	static TextureHandle prefilteredEnvMap(TextureManager& textureManager, VulkanDevice& vulkanDevice,
	                                       TextureHandle envCubemap, DescriptorManager& descriptorManager,
	                                       BindlessTextureDSetComponent& dSetComponent, PipelineManager& pipelineManager);
	static TextureHandle brdfLut(TextureManager& textureManager, VulkanDevice& vulkanDevice, DescriptorManager& descriptorManager,
	                             BindlessTextureDSetComponent& dSetComponent, PipelineManager& pipelineManager);
};
