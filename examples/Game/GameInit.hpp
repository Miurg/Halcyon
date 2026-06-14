#pragma once

#include <Halcyon.hpp>

using Orhescyon::GeneralManager;
class GameInit : public IStartUp
{
public:
	void Run(GeneralManager& gm);
private:
	static void coreInit(GeneralManager& gm);
	static void initScene(GeneralManager& gm);
};
