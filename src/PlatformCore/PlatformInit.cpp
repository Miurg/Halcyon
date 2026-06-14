#include "PlatformInit.hpp"
#include <iostream>
#include "Systems/InputSolverSystem.hpp"
#include "PlatformCore/PlatformContexts.hpp"
#include "PlatformCore/Window.hpp"
#include "PlatformCore/Components/WindowComponent.hpp"
#include "PlatformCore/Components/KeyboardStateComponent.hpp"
#include "PlatformCore/Components/MouseStateComponent.hpp"
#include "PlatformCore/Components/CursorPositionComponent.hpp"
#include "PlatformCore/Components/ScrollDeltaComponent.hpp"
#include "PlatformCore/Components/WindowSizeComponent.hpp"
#include "GraphicsCore/Components/NameComponent.hpp"

#include "DeletionQueueComponent.hpp"
#include "DeletionQueueContext.hpp"

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
	gm.registerSystem<InputSolverSystem>();
}
#pragma endregion

#pragma region initPlatform
void PlatformInit::initPlatform(Orhescyon::GeneralManager& gm)
{
	DeletionQueue* dq = gm.getContextComponent<DeletionQueueContext, DeletionQueueComponent>()->queue;

	Orhescyon::Entity windowAndInputEntity = gm.createEntity();
	gm.registerContext<InputDataContext>(windowAndInputEntity);
	gm.registerContext<MainWindowContext>(windowAndInputEntity);
	Window* window = new Window("Halcyon");
	gm.addComponent<WindowComponent>(windowAndInputEntity, window);
	dq->push_function([window]() { delete window; });
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
