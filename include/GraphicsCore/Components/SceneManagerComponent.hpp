#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/SceneManager.hpp"

struct HALCYON_API SceneManagerComponent
{
	SceneManager* sceneManager;

	SceneManagerComponent(SceneManager* sceneManager) : sceneManager(sceneManager) {}
};
