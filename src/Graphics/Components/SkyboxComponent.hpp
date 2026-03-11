#pragma once

#include "Resources/Managers/ResourceHandles.hpp"
#include <vulkan/vulkan.hpp>

struct SkyboxComponent
{
	TextureHandle cubemapTexture;
	TextureHandle irradianceMap;
	TextureHandle prefilteredMap;
	TextureHandle brdfLut;
};
