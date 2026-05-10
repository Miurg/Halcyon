#pragma once
#include <Orhescyon/GeneralManager.hpp>

class PlatformInit
{
public:
	static void Run(Orhescyon::GeneralManager& gm);
private:
	static void coreInit(Orhescyon::GeneralManager& gm);
};
