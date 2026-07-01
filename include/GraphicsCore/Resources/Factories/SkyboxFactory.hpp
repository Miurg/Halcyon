#pragma once

#include "HalcyonExport.hpp"
#include <string>
#include <Orhescyon/GeneralManager.hpp>

using Orhescyon::GeneralManager;

class HALCYON_API SkyboxFactory
{
public:
	static void loadSkybox(const std::string& hdrPath, GeneralManager& gm);
};
