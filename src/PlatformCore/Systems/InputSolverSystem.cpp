#include "PlatformCore/Systems/InputSolverSystem.hpp"

#include <GLFW/glfw3.h>
#include "PlatformCore/PlatformContexts.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

void InputSolverSystem::update(GeneralManager& gm)
{
#ifdef TRACY_ENABLE
	ZoneScopedN("InputSolverSystem");
#endif

	forEachSubscribedEntity(
	    gm,
	    [](Orhescyon::Entity, WindowComponent& window, CursorPositionComponent& cursorPosition,
	       WindowSizeComponent& windowSize, KeyboardStateComponent& keyboardState, MouseStateComponent& mouseState,
	       ScrollDeltaComponent& scrollDelta)
	    {
		    Window* mainWindow = window.windowInstance;
		    while (!mainWindow->inputQueue.empty())
		    {
			    const auto& e = mainWindow->inputQueue.front();

			    switch (e.Type)
			    {
			    case InputEvent::Type::Key:
				    keyboardState.keys[e.key] = (e.action == GLFW_PRESS || e.action == GLFW_REPEAT);
				    break;

			    case InputEvent::Type::MouseButton:
				    mouseState.keys[e.key] = (e.action == GLFW_PRESS || e.action == GLFW_REPEAT);
				    break;

			    case InputEvent::Type::MouseMove:
				    cursorPosition.mousePositionX = e.mousePositionX;
				    cursorPosition.mousePositionY = e.mousePositionY;
				    break;

			    case InputEvent::Type::MouseScroll:
				    scrollDelta.deltaScrollX += e.deltaScrollX;
				    scrollDelta.deltaScrollY += e.deltaScrollY;
				    break;

			    case InputEvent::Type::WindowResize:
				    windowSize.windowWidth = e.windowWidth;
				    windowSize.windowHeight = e.windowHeight;
				    break;

			    default:
				    break;
			    }

			    mainWindow->inputQueue.pop();
		    }
	    });
}

void InputSolverSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "InputSolverSystem registered!" << std::endl;
};

void InputSolverSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "InputSolverSystem shutdown!" << std::endl;
};