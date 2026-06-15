#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"
#include <vulkan/vulkan.hpp>

struct HALCYON_API SkyboxComponent
{
	TextureHandle cubemapTexture;
	TextureHandle prefilteredMap;
	TextureHandle brdfLut;
	bool hasSkybox = false;
};
