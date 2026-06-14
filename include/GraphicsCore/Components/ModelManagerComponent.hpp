#pragma once

#include "GraphicsCore/Resources/Managers/ModelManager.hpp"

struct ModelManagerComponent
{
	ModelManager* modelManager;

	ModelManagerComponent(ModelManager* modelManager) : modelManager(modelManager) {}
};