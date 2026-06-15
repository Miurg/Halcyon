#pragma once

#include "HalcyonExport.hpp"
#include "PhysicsCore/Managers/PhysManager.hpp"

struct HALCYON_API PhysManagerComponent
{
	PhysManager* physManager = nullptr;
};