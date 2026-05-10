#include "PlatformInit.hpp"
#include <iostream>
#include "Systems/DeltaTimeSystem.hpp"
#include "Systems/InputSolverSystem.hpp"
#include "PlatformContexts.hpp"
#include "Window.hpp"
#include "Components/DeltaTimeComponent.hpp"
#include "Components/WindowComponent.hpp"
#include "Components/KeyboardStateComponent.hpp"
#include "Components/MouseStateComponent.hpp"
#include "Components/CursorPositionComponent.hpp"
#include "Components/ScrollDeltaComponent.hpp"
#include "Components/WindowSizeComponent.hpp"
#include "../Graphics/Components/NameComponent.hpp"

#pragma region Run
void PlatformInit::Run(Orhescyon::GeneralManager& gm)
{
#ifdef _DEBUG
	std::cout << "PLATFORMINIT::RUN::Start init" << std::endl;
#endif //_DEBUG

	coreInit(gm);
	initPlatform(gm);

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

#pragma region initPlatform
void PlatformInit::initPlatform(Orhescyon::GeneralManager& gm)
{
	Entity deltaTimeEntity = gm.createEntity();
	gm.registerContext<DeltaTimeContext>(deltaTimeEntity);
	gm.addComponent<DeltaTimeComponent>(deltaTimeEntity);
	gm.addComponent<NameComponent>(deltaTimeEntity, "SYSTEM::PLATFORM Delta Time");

	Entity windowAndInputEntity = gm.createEntity();
	gm.registerContext<InputDataContext>(windowAndInputEntity);
	gm.registerContext<MainWindowContext>(windowAndInputEntity);
	Window* window = new Window("Halcyon");
	gm.addComponent<WindowComponent>(windowAndInputEntity, window);
	gm.addComponent<KeyboardStateComponent>(windowAndInputEntity);
	gm.addComponent<MouseStateComponent>(windowAndInputEntity);
	gm.addComponent<CursorPositionComponent>(windowAndInputEntity);
	gm.addComponent<NameComponent>(windowAndInputEntity, "SYSTEM::PLATFORM Window and Input");
	unsigned int ScreenWidth = 1920;
	unsigned int ScreenHeight = 1080;
	gm.addComponent<WindowSizeComponent>(windowAndInputEntity, ScreenWidth, ScreenHeight);
	gm.addComponent<ScrollDeltaComponent>(windowAndInputEntity);
	gm.subscribeEntity<InputSolverSystem>(windowAndInputEntity);
}
#pragma endregion
