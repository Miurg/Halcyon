#pragma once

#include "HalcyonExport.hpp"
#include "AudioCore/Managers/AudioManager.hpp"

struct HALCYON_API AudioManagerComponent
{
	AudioManager* audioManager = nullptr;
};