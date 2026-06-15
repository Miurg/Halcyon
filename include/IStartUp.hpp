#pragma once

#include <Orhescyon/GeneralManager.hpp>

struct IStartUp
{
	virtual void Run(Orhescyon::GeneralManager& gm) = 0;
	virtual ~IStartUp() = default;
};