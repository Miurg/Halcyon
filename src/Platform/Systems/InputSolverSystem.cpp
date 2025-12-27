#include "InputSolverSystem.hpp"

#include <GLFW/glfw3.h>
#include "../PlatformContexts.hpp"

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
}
void InputSolverSystem::onRegistered(GeneralManager& gm) 
{
	std::cout << "InputSolverSystem registered!" << std::endl;
	Entity windowAndInputEntity = gm.createEntity();
	gm.registerContext<InputDataContext>(windowAndInputEntity);
	gm.registerContext<MainWindowContext>(windowAndInputEntity);
	Window* window = new Window("Halcyon");
	gm.addComponent<WindowComponent>(windowAndInputEntity, window);
	gm.addComponent<KeyboardStateComponent>(windowAndInputEntity);
	gm.addComponent<MouseStateComponent>(windowAndInputEntity);
	gm.addComponent<CursorPositionComponent>(windowAndInputEntity);
	unsigned int ScreenWidth = 1920;
	unsigned int ScreenHeight = 1080;
	gm.addComponent<WindowSizeComponent>(windowAndInputEntity, ScreenWidth, ScreenHeight);
	gm.addComponent<ScrollDeltaComponent>(windowAndInputEntity);
	gm.subscribeEntity<InputSolverSystem>(windowAndInputEntity);
	std::cout << windowAndInputEntity << std::endl;
};

void InputSolverSystem::onShutdown(GeneralManager& gm) 
{
	std::cout << "InputSolverSystem shutdown!" << std::endl;
	gm.getContextComponent<MainWindowContext, WindowComponent>()->windowInstance->~Window();
};