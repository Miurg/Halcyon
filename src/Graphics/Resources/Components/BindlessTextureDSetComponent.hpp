#pragma once
#include <vulkan/vulkan_raii.hpp>

struct BindlessTextureDSetComponent
{
	int bindlessTextureSet = -1;
	int bindlessTextureBuffer = -1;
};