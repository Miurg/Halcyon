#pragma once
#include <GLFW/glfw3.h>
#include "SimulationCore/Contexts/MainCameraContext.h"
#include "../../GLFWCore/Contexts/InputDataContext.h"
#include "../../RenderCore/Camera.h"
class ControlSystem : public SystemContextual<ControlSystem>
{
private:
	bool WireframeToggle = false;
	bool CursorDisableToggle = true;
	double LastMousePositionX = 0.0;
	double LastMousePositionY = 0.0;


public:
	void Update(float deltaTime, GeneralManager& gm) override
	{
		KeyboardStateComponent* keyboardState =
		    gm.GetComponent<KeyboardStateComponent>(gm.GetContext<InputDataContext>()->InputInstance);
		CursorPositionComponent* cursorPositionState =
		    gm.GetComponent<CursorPositionComponent>(gm.GetContext<InputDataContext>()->InputInstance);
		WindowComponent* windowInstance =
		    gm.GetComponent<WindowComponent>(gm.GetContext<MainWindowContext>()->WindowInstance);
		CameraComponent* mainCamera = gm.GetComponent<CameraComponent>(gm.GetContext<MainCameraContext>()->CameraInstance);

		if (keyboardState->Keys[GLFW_KEY_ESCAPE])
		{
			glfwSetWindowShouldClose(windowInstance->WindowInstance->GetHandle(), true);
		}
		if (keyboardState->Keys[GLFW_KEY_1])
		{
			if (WireframeToggle)
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				WireframeToggle = !WireframeToggle;
				std::cout << "Wireframe" << std::endl;
			}
			else
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				WireframeToggle = !WireframeToggle;
			}
		}
		if (keyboardState->Keys[GLFW_KEY_2])
		{
			if (CursorDisableToggle)
			{
				glfwSetInputMode(windowInstance->WindowInstance->GetHandle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				CursorDisableToggle = !CursorDisableToggle;
			}
			else
			{
				glfwSetInputMode(windowInstance->WindowInstance->GetHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				CursorDisableToggle = !CursorDisableToggle;
			}
		}
		if (CursorDisableToggle)
		{
			constexpr GLfloat sensitivity = 0.1f;
			GLfloat xoffset = static_cast<GLfloat>((cursorPositionState->MousePositionX - LastMousePositionX) * sensitivity);
			GLfloat yoffset = static_cast<GLfloat>((LastMousePositionY - cursorPositionState->MousePositionY) * sensitivity);
			LastMousePositionX = cursorPositionState->MousePositionX;
			LastMousePositionY = cursorPositionState->MousePositionY;
			mainCamera->CameraInstance->ProcessMouseMovement(xoffset, yoffset, true);
		}
	}
};