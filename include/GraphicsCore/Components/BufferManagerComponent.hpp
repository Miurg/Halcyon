#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/BufferManager.hpp"

struct HALCYON_API BufferManagerComponent
{
	BufferManager* bufferManager;

	BufferManagerComponent(BufferManager* manager) : bufferManager(manager) {}
};