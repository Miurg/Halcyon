#pragma once

#include "../Resources/Managers/DescriptorManager.hpp"

struct DescriptorManagerComponent
{
	DescriptorManager* descriptorManager;

	DescriptorManagerComponent(DescriptorManager* manager) : descriptorManager(manager) {}
};