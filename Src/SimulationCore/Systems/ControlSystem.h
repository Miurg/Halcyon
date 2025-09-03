#pragma once
#include <GLFW/glfw3.h>
#include "SimulationCore/Contexts/MainCameraContext.h"
#include "../../GLFWCore/Contexts/InputDataContext.h"
#include "../../RenderCore/Components/CameraComponent.h"
#include "../../GLFWCore/Components/WindowComponent.h"
#include "../../GLFWCore/Components/KeyboardStateComponent.h"
#include "../../GLFWCore/Components/CursorPositionComponent.h"
#include "../../GLFWCore/Contexts/MainWIndowContext.h"
class ControlSystem : public SystemContextual<ControlSystem>
{
private:
	bool Wireframe = true;
	bool WireframeKey = false;
	bool CursorDisable = true;
	bool CursorDisableKey = false;
	double LastMousePositionX = 0.0;
	double LastMousePositionY = 0.0;

	void WireFrameToggle()
	{
		if (Wireframe)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			Wireframe = !Wireframe;
			std::cout << "Wireframe" << std::endl;
		}
		else
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			Wireframe = !Wireframe;
		}
	}

	void CursorDisableToggle()
	{
		if (CursorDisable)
		{
			glfwSetInputMode(glfwGetCurrentContext(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			CursorDisable = !CursorDisable;
		}
		else
		{
			glfwSetInputMode(glfwGetCurrentContext(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			CursorDisable = !CursorDisable;
		}
	}

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
			if (WireframeKey)
			{
				WireFrameToggle();
				WireframeKey = false;
			}
		}
		else
		{
			WireframeKey = true;
		}
		if (keyboardState->Keys[GLFW_KEY_2])
		{
			if (CursorDisableKey)
			{
				CursorDisableToggle();
				CursorDisableKey = false;
			}
		}
		else
		{
			CursorDisableKey = true;
		}
		if (CursorDisable)
		{
			//=== Mouse ===
			constexpr GLfloat sensitivity = 0.1f;
			GLfloat xoffset = static_cast<GLfloat>((cursorPositionState->MousePositionX - LastMousePositionX) * sensitivity);
			GLfloat yoffset = static_cast<GLfloat>((LastMousePositionY - cursorPositionState->MousePositionY) * sensitivity);
			LastMousePositionX = cursorPositionState->MousePositionX;
			LastMousePositionY = cursorPositionState->MousePositionY;
			xoffset *= mainCamera->MouseSensitivity;
			yoffset *= mainCamera->MouseSensitivity;

			mainCamera->Yaw += xoffset;
			mainCamera->Pitch += yoffset;
			if (mainCamera->Pitch > 89.0f) mainCamera->Pitch = 89.0f;
			if (mainCamera->Pitch < -89.0f) mainCamera->Pitch = -89.0f;
			//=== Keyboard ===
			GLfloat velocity = mainCamera->MovementSpeed * deltaTime;
			if (keyboardState->Keys[GLFW_KEY_W]) mainCamera->Position += mainCamera->Front * velocity;
			if (keyboardState->Keys[GLFW_KEY_S]) mainCamera->Position -= mainCamera->Front * velocity;
			if (keyboardState->Keys[GLFW_KEY_A]) mainCamera->Position -= mainCamera->Right * velocity;
			if (keyboardState->Keys[GLFW_KEY_D]) mainCamera->Position += mainCamera->Right * velocity;

			mainCamera->UpdateCameraVectors();
		}
	}
};