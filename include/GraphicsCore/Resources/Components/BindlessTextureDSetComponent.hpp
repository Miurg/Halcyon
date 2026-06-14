#pragma once
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"

struct BindlessTextureDSetComponent
{
	DSetHandle bindlessTextureSet;
	BufferHandle materialBuffer;
};