#pragma once

#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"
#include <vulkan/vulkan.hpp>

struct SkyboxComponent
{
	TextureHandle cubemapTexture;
	TextureHandle prefilteredMap;
	TextureHandle brdfLut;
	bool hasSkybox = false;
};
