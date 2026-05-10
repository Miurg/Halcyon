#pragma once

#include <Orhescyon/GeneralManager.hpp>
using Orhescyon::GeneralManager;
class GameInit
{
public:
	static void Run(GeneralManager& gm);
private:
	static void coreInit(GeneralManager& gm);
	static void initScene(GeneralManager& gm);
};
