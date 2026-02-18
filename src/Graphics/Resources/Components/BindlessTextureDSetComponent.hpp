#pragma once
#include "../Managers/ResourceHandles.hpp"

struct BindlessTextureDSetComponent
{
	DSetHandle bindlessTextureSet;
	TextureHandle bindlessTextureBuffer;
	BufferHandle materialBuffer;
};