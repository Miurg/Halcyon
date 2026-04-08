

#pragma once
#include <Orhescyon/GeneralManager.hpp>

using Orhescyon::GeneralManager;

// Bakes a uniform grid of SH light probes.
// Slot 0 (skybox fallback) is untouched - baking starts at slot 1.
class LightProbeGIBaking
{
public:
    static void bakeAll(GeneralManager& gm);
};
