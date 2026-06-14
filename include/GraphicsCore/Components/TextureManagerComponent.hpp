#pragma once

#include "GraphicsCore/Resources/Managers/TextureManager.hpp"

struct TextureManagerComponent
{
	TextureManager* textureManager;

	TextureManagerComponent(TextureManager* manager) : textureManager(manager) {}
};