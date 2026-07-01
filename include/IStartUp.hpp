#pragma once

#include "HalcyonExport.hpp"
#include <Orhescyon/GeneralManager.hpp>

struct HALCYON_API IStartUp
{
	virtual void Run(Orhescyon::GeneralManager& gm) = 0;
	virtual ~IStartUp() = default;
};