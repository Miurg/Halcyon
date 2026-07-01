#pragma once

#include "HalcyonExport.hpp"
struct HALCYON_API AutoExposureSettingsComponent
{
    float tauUp = 0.5f;
    float tauDown = 0.5f;
    float minEV = -1.5f;
    float maxEV = 6.5f;
    float targetLuminance = 0.3f;
    float lowPercent = 0.5f;
    float highPercent = 0.95f;

    AutoExposureSettingsComponent() = default;
};
