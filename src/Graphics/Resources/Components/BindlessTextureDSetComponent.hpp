#pragma once
#include "../Managers/ResourceHandles.hpp"

struct BindlessTextureDSetComponent
{
	DSetHandle bindlessTextureSet;
	BufferHandle materialBuffer;
};