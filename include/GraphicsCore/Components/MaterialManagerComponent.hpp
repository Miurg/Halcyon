#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/MaterialManager.hpp"

struct HALCYON_API MaterialManagerComponent
{
	MaterialManager* materialManager;

	MaterialManagerComponent(MaterialManager* manager) : materialManager(manager) {}
};
