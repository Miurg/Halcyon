#pragma once

#include "../../Core/Systems/System.h"
#include "../Components/CursorPositionComponent.h"
#include "../Components/WindowSizeComponent.h"
#include "../Components/KeyboardStateComponent.h"
#include "../Components/MouseStateComponent.h"
#include "../Components/ScrollDeltaComponent.h"
#include "../Components/WindowComponent.h"

class InputSolverSystem : public System<InputSolverSystem, WindowComponent, CursorPositionComponent,
                                        WindowSizeComponent, KeyboardStateComponent, MouseStateComponent,
                                        ScrollDeltaComponent>
{
public:
	void ProcessEntity(Entity entity, ComponentManager& cm, ContextManager& ctxM, float deltaTime)
	{
		CursorPositionComponent* cursorPosition = cm.GetComponent<CursorPositionComponent>(entity);
		WindowSizeComponent* windowSize = cm.GetComponent<WindowSizeComponent>(entity);
		KeyboardStateComponent* keyboardState = cm.GetComponent<KeyboardStateComponent>(entity);
		MouseStateComponent* mouseState = cm.GetComponent<MouseStateComponent>(entity);
		ScrollDeltaComponent* scrollDelta = cm.GetComponent<ScrollDeltaComponent>(entity);
		Window* mainWindow = cm.GetComponent<WindowComponent>(entity)->WindowInstance;
		while (!mainWindow->InputQueue.empty())
		{
			const auto& e = mainWindow->InputQueue.front();

			switch (e.Type)
			{
			case InputEvent::Type::Key:
				keyboardState->Keys[e.Key] = (e.Action != GLFW_RELEASE);
				break;

			case InputEvent::Type::MouseButton:
				mouseState->Keys[e.Key] = (e.Action != GLFW_RELEASE);
				break;

			case InputEvent::Type::MouseMove:
				cursorPosition->DeltaPositionX = e.MousePositionX - cursorPosition->MousePositionX;
				cursorPosition->DeltaPositionY = e.MousePositionY - cursorPosition->MousePositionY;
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