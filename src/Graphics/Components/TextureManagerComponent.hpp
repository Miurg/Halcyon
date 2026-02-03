#pragma once

#include "../Resources/Managers/TextureManager.hpp"

struct TextureManagerComponent
{
	TextureManager* textureManager;

	TextureManagerComponent(TextureManager* manager) : textureManager(manager) {}
};