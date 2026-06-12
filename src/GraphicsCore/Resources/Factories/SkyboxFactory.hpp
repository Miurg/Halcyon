#pragma once

#include <string>
#include <Orhescyon/GeneralManager.hpp>

using Orhescyon::GeneralManager;

class SkyboxFactory
{
public:
	static void loadSkybox(const std::string& hdrPath, GeneralManager& gm);
};
