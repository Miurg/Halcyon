#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/TextureManager.hpp"

struct HALCYON_API TextureManagerComponent
{
	TextureManager* textureManager;

	TextureManagerComponent(TextureManager* manager) : textureManager(manager) {}
};