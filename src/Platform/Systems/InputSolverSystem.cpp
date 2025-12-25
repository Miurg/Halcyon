#include "InputSolverSystem.hpp"

#include <GLFW/glfw3.h>

void InputSolverSystem::processEntity(Entity entity, GeneralManager& gm, float deltaTime)
{
	CursorPositionComponent* cursorPosition = gm.getComponent<CursorPositionComponent>(entity);
	WindowSizeComponent* windowSize = gm.getComponent<WindowSizeComponent>(entity);
	KeyboardStateComponent* keyboardState = gm.getComponent<KeyboardStateComponent>(entity);
	MouseStateComponent* mouseState = gm.getComponent<MouseStateComponent>(entity);
	ScrollDeltaComponent* scrollDelta = gm.getComponent<ScrollDeltaComponent>(entity);
	Window* mainWindow = gm.getComponent<WindowComponent>(entity)->windowInstance;
	while (!mainWindow->inputQueue.empty())
	{
		const auto& e = mainWindow->inputQueue.front();

		switch (e.Type)
		{
		case InputEvent::Type::Key:
			keyboardState->keys[e.key] = (e.action == GLFW_PRESS || e.action == GLFW_REPEAT);
			break;

		case InputEvent::Type::MouseButton:
			mouseState->keys[e.key] = (e.action == GLFW_PRESS || e.action == GLFW_REPEAT);
			break;

		case InputEvent::Type::MouseMove:
			cursorPosition->mousePositionX = e.mousePositionX;
			cursorPosition->mousePositionY = e.mousePositionY;
			break;

		case InputEvent::Type::MouseScroll:
			scrollDelta->deltaScrollX += e.deltaScrollX;
			scrollDelta->deltaScrollY += e.deltaScrollY;
			break;

		case InputEvent::Type::WindowResize:
			windowSize->windowWidth = e.windowWidth;
			windowSize->windowHeight = e.windowHeight;
			break;

		default:
			break;
		}

		mainWindow->inputQueue.pop();
	}
};