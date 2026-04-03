#pragma once

#include "../Resources/Managers/ResourceHandles.hpp"
#include <vulkan/vulkan.hpp>

struct SkyboxComponent
{
	TextureHandle cubemapTexture;
	BufferHandle  shBuffer;
	TextureHandle prefilteredMap;
	TextureHandle brdfLut;
	bool hasSkybox = false;
};
