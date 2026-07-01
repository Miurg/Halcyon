#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"

struct HALCYON_API BindlessTextureDSetComponent
{
	DSetHandle bindlessTextureSet;
	BufferHandle materialBuffer;
};