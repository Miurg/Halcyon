#include "PlatformInit.hpp"
#include <iostream>
#include "PlatformCore/Systems/InputSolverSystem.hpp"
#include "PlatformCore/PlatformContexts.hpp"
#include "PlatformCore/Window.hpp"
#include "PlatformCore/Components/WindowComponent.hpp"
#include "PlatformCore/Components/KeyboardStateComponent.hpp"
#include "PlatformCore/Components/MouseStateComponent.hpp"
#include "PlatformCore/Components/CursorPositionComponent.hpp"
#include "PlatformCore/Components/ScrollDeltaComponent.hpp"
#include "PlatformCore/Components/WindowSizeComponent.hpp"
#include "GraphicsCore/Components/NameComponent.hpp"
#include "GraphicsCore/Systems/DeltaTimeSystem.hpp"
#include "GraphicsCore/Systems/FrameBeginSystem.hpp"

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
	gm.registerSystem<InputSolverSystem>()
	    .after<DeltaTimeSystem>()
	    .before<FrameBeginSystem>()
	    .writes<KeyboardStateComponent, MouseStateComponent, CursorPositionComponent, ScrollDeltaComponent,
	            WindowSizeComponent>();
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
	gm.addComponentImmediate<WindowComponent>(windowAndInputEntity, window);
	dq->push_function([window]() { delete window; });
	gm.addComponentImmediate<KeyboardStateComponent>(windowAndInputEntity);
	gm.addComponentImmediate<MouseStateComponent>(windowAndInputEntity);
	gm.addComponentImmediate<CursorPositionComponent>(windowAndInputEntity);
	gm.addComponentImmediate<NameComponent>(windowAndInputEntity, "SYSTEM::PLATFORM Window and Input");
	unsigned int ScreenWidth = 1920;
	unsigned int ScreenHeight = 1080;
	gm.addComponentImmediate<WindowSizeComponent>(windowAndInputEntity, ScreenWidth, ScreenHeight);
	gm.addComponentImmediate<ScrollDeltaComponent>(windowAndInputEntity);
	gm.subscribeEntityImmediate<InputSolverSystem>(windowAndInputEntity);
}
#pragma endregion
