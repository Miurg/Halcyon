#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"

struct HALCYON_API DescriptorManagerComponent
{
	DescriptorManager* descriptorManager;

	DescriptorManagerComponent(DescriptorManager* manager) : descriptorManager(manager) {}
};