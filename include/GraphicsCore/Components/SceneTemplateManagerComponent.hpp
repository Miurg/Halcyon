#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/SceneTemplateManager.hpp"

struct HALCYON_API SceneTemplateManagerComponent
{
	SceneTemplateManager* sceneTemplateManager;

	SceneTemplateManagerComponent(SceneTemplateManager* sceneTemplateManager)
	    : sceneTemplateManager(sceneTemplateManager)
	{
	}
};
