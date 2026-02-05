#pragma once

#include "../Managers/FrameManager.hpp"

struct FrameManagerComponent
{
	FrameManager* frameManager;

	FrameManagerComponent(FrameManager* manager) : frameManager(manager) {}
};