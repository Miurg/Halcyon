#include "ControlSystem.hpp"
#include "../../Graphics/Components/LocalTransformComponent.hpp"
#include "../../Graphics/Components/CameraComponent.hpp"
#include "../../Platform/Components/KeyboardStateComponent.hpp"
#include "../../Platform/Components/CursorPositionComponent.hpp"
#include <GLFW/glfw3.h>
#include "../../Graphics/GraphicsContexts.hpp"
#include "../../Platform/PlatformContexts.hpp"
#include "../Components/ControlComponent.hpp"
#include "../../Platform/Components/WindowComponent.hpp"
#include "../../Core/GeneralManager.hpp"
#include "../../Platform/Window.hpp"
#include "../../Graphics/Components/GlobalTransformComponent.hpp"

void ControlSystem::cursorDisableToggle(Window* window)
{
	GLFWwindow* windowHandle = window->getHandle();
	if (cursorDisable)
	{
		glfwSetInputMode(windowHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		cursorDisable = !cursorDisable;
	}
	else
	{
		glfwSetInputMode(windowHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		cursorDisable = !cursorDisable;
	}
}

void ControlSystem::update(float deltaTime, GeneralManager& gm)
{
	KeyboardStateComponent* keyboardState = gm.getContextComponent<InputDataContext, KeyboardStateComponent>();
	CursorPositionComponent* cursorPositionState = gm.getContextComponent<InputDataContext, CursorPositionComponent>();
	WindowComponent* windowInstance = gm.getContextComponent<MainWindowContext, WindowComponent>();
	CameraComponent* mainCamera = gm.getContextComponent<MainCameraContext, CameraComponent>();
	GlobalTransformComponent* mainCameraTransform =
	    gm.getContextComponent<MainCameraContext, GlobalTransformComponent>();
	ControlComponent* mainCameraControl = gm.getContextComponent<MainCameraContext, ControlComponent>();

	if (keyboardState->keys[GLFW_KEY_ESCAPE])
	{
		glfwSetWindowShouldClose(windowInstance->windowInstance->getHandle(), true);
	}
	if (keyboardState->keys[GLFW_KEY_2])
	{
		if (cursorDisableKey)
		{
			cursorDisableToggle(windowInstance->windowInstance);
			cursorDisableKey = false;
		}
	}
	else
	{
		cursorDisableKey = true;
	}
	if (cursorDisable)
	{
		//=== Mouse ===
		constexpr float sensitivity = 0.1f;
		float xoffset = static_cast<float>((cursorPositionState->mousePositionX - lastMousePositionX) * sensitivity);
		float yoffset = static_cast<float>((cursorPositionState->mousePositionY - lastMousePositionY) * sensitivity);
		lastMousePositionX = cursorPositionState->mousePositionX;
		lastMousePositionY = cursorPositionState->mousePositionY;
		xoffset *= mainCameraControl->mouseSensitivity;
		yoffset *= mainCameraControl->mouseSensitivity;
		mainCameraTransform->rotateGlobal(glm::radians(-xoffset), glm::vec3(0.0f, 1.0f, 0.0f));
		mainCameraTransform->rotateLocal(glm::radians(-yoffset), glm::vec3(1.0f, 0.0f, 0.0f));
		//=== Keyboard ===
		float velocity = mainCameraControl->movementSpeed * deltaTime;
		if (keyboardState->keys[GLFW_KEY_W]) mainCameraTransform->globalPosition += mainCameraTransform->front * velocity;
		if (keyboardState->keys[GLFW_KEY_S]) mainCameraTransform->globalPosition -= mainCameraTransform->front * velocity;
		if (keyboardState->keys[GLFW_KEY_A]) mainCameraTransform->globalPosition -= mainCameraTransform->right * velocity;
		if (keyboardState->keys[GLFW_KEY_D]) mainCameraTransform->globalPosition += mainCameraTransform->right * velocity;
	}
}