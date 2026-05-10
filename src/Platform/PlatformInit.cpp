#include "PlatformInit.hpp"
#include <iostream>
#include "Systems/DeltaTimeSystem.hpp"
#include "Systems/InputSolverSystem.hpp"

#pragma region Run
void PlatformInit::Run(Orhescyon::GeneralManager& gm)
{
#ifdef _DEBUG
	std::cout << "PLATFORMINIT::RUN::Start init" << std::endl;
#endif //_DEBUG

	coreInit(gm);

#ifdef _DEBUG
	std::cout << "PLATFORMINIT::RUN::Succes!" << std::endl;
#endif //_DEBUG
}
#pragma endregion

#pragma region coreInit
void PlatformInit::coreInit(Orhescyon::GeneralManager& gm)
{
	gm.registerSystem<DeltaTimeSystem>();
	gm.registerSystem<InputSolverSystem>();
}
#pragma endregion
