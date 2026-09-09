#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/RenderAssetManager.hpp"

struct HALCYON_API RenderAssetManagerComponent
{
	RenderAssetManager* renderAssetManager;

	RenderAssetManagerComponent(RenderAssetManager* renderAssetManager) : renderAssetManager(renderAssetManager) {}
};
