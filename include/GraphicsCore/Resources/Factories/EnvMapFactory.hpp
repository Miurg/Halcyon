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
	static TextureHandle cubemapFromHdr(TextureManager& tManager, VulkanDevice& vulkanDevice, TextureHandle hdrTexture,
	                                    DescriptorManager& dManager, BindlessTextureDSetComponent& dSetComponent,
	                                    PipelineManager& pManager);
	static TextureHandle prefilteredEnvMap(TextureManager& tManager, VulkanDevice& vulkanDevice,
	                                       TextureHandle envCubemap, DescriptorManager& dManager,
	                                       BindlessTextureDSetComponent& dSetComponent, PipelineManager& pManager);
	static TextureHandle brdfLut(TextureManager& tManager, VulkanDevice& vulkanDevice, DescriptorManager& dManager,
	                             BindlessTextureDSetComponent& dSetComponent, PipelineManager& pManager);
};
