#pragma once

#include "../../Core/Systems/SystemSubscribed.hpp"
#include "../Components/CursorPositionComponent.hpp"
#include "../Components/WindowSizeComponent.hpp"
#include "../Components/KeyboardStateComponent.hpp"
#include "../Components/MouseStateComponent.hpp"
#include "../Components/ScrollDeltaComponent.hpp"
#include "../Components/WindowComponent.hpp"

class InputSolverSystem
    : public SystemSubscribed<InputSolverSystem, WindowComponent, CursorPositionComponent, WindowSizeComponent,
                              KeyboardStateComponent, MouseStateComponent, ScrollDeltaComponent>
{
public:
	void processEntity(Entity entity, GeneralManager& gm, float deltaTime)
	{
		CursorPositionComponent* cursorPosition = gm.getComponent<CursorPositionComponent>(entity);
		WindowSizeComponent* windowSize = gm.getComponent<WindowSizeComponent>(entity);
		KeyboardStateComponent* keyboardState = gm.getComponent<KeyboardStateComponent>(entity);
		MouseStateComponent* mouseState = gm.getComponent<MouseStateComponent>(entity);
		ScrollDeltaComponent* scrollDelta = gm.getComponent<ScrollDeltaComponent>(entity);
		Window* mainWindow = gm.getComponent<WindowComponent>(entity)->WindowInstance;
		std::cout << "InputSolverSystem processing entity " << entity << std::endl;
		while (!mainWindow->InputQueue.empty())
		{
			const auto& e = mainWindow->InputQueue.front();

			switch (e.Type)
			{
			case InputEvent::Type::Key:
				keyboardState->Keys[e.Key] = (e.Action == GLFW_PRESS || e.Action == GLFW_REPEAT);
				break;

			case InputEvent::Type::MouseButton:
				mouseState->Keys[e.Key] = (e.Action == GLFW_PRESS || e.Action == GLFW_REPEAT);
				break;

			case InputEvent::Type::MouseMove:
				cursorPosition->MousePositionX = e.MousePositionX;
				cursorPosition->MousePositionY = e.MousePositionY;
				break;

			case InputEvent::Type::MouseScroll:
				scrollDelta->DeltaScrollX += e.DeltaScrollX;
				scrollDelta->DeltaScrollY += e.DeltaScrollY;
				break;

			case InputEvent::Type::WindowResize:
				windowSize->WindowWidth = e.WindowWidth;
				windowSize->WindowHeight = e.WindowHeight;
				break;

			default:
				break;
			}

			mainWindow->InputQueue.pop();
		}
	};
};