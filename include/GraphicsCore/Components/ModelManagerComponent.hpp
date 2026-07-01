#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/ModelManager.hpp"

struct HALCYON_API ModelManagerComponent
{
	ModelManager* modelManager;

	ModelManagerComponent(ModelManager* modelManager) : modelManager(modelManager) {}
};