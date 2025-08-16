#pragma once

#include "../../Core/Systems/SystemSubscribed.h"
#include "../Components/CursorPositionComponent.h"
#include "../Components/WindowSizeComponent.h"
#include "../Components/KeyboardStateComponent.h"
#include "../Components/MouseStateComponent.h"
#include "../Components/ScrollDeltaComponent.h"
#include "../Components/WindowComponent.h"

class InputSolverSystem : public SystemSubscribed<InputSolverSystem, WindowComponent, CursorPositionComponent,
                                        WindowSizeComponent, KeyboardStateComponent, MouseStateComponent,
                                        ScrollDeltaComponent>
{
public:
	void ProcessEntity(Entity entity, GeneralManager& gm, float deltaTime)
	{
		CursorPositionComponent* cursorPosition = gm.GetComponent<CursorPositionComponent>(entity);
		WindowSizeComponent* windowSize = gm.GetComponent<WindowSizeComponent>(entity);
		KeyboardStateComponent* keyboardState = gm.GetComponent<KeyboardStateComponent>(entity);
		MouseStateComponent* mouseState = gm.GetComponent<MouseStateComponent>(entity);
		ScrollDeltaComponent* scrollDelta = gm.GetComponent<ScrollDeltaComponent>(entity);
		Window* mainWindow = gm.GetComponent<WindowComponent>(entity)->WindowInstance;
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