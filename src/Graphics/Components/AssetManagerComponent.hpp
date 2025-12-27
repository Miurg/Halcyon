#pragma once

#include <memory>
#include "../Resources/Managers/AssetManager.hpp"

struct AssetManagerComponent
{
	AssetManager* assetManager;

	AssetManagerComponent(AssetManager* manager) : assetManager(manager) {}
};