#pragma once
#include "../../Core/Components/ComponentManager.h"
#include "../Shader.h"
#include "../AssetManager.h"

struct AssetManagerComponent : Component
{
	std::unique_ptr<AssetManager> AssetManagerInstance;
	AssetManagerComponent() : AssetManagerInstance(std::make_unique<AssetManager>()) {}
};