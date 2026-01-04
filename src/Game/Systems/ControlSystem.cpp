#include "ControlSystem.hpp"

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
	KeyboardStateComponent* keyboardState =
	    gm.getContextComponent<InputDataContext, KeyboardStateComponent>();
	CursorPositionComponent* cursorPositionState =
	    gm.getContextComponent<InputDataContext, CursorPositionComponent>();
	WindowComponent* windowInstance =
	    gm.getContextComponent<MainWindowContext, WindowComponent>();
	CameraComponent* mainCamera =
	    gm.getContextComponent<MainCameraContext, CameraComponent>();

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
		xoffset *= mainCamera->mouseSensitivity;
		yoffset *= mainCamera->mouseSensitivity;
		mainCamera->rotate(xoffset, -yoffset);
		//=== Keyboard ===
		float velocity = mainCamera->movementSpeed * deltaTime;
		if (keyboardState->keys[GLFW_KEY_W]) mainCamera->position += mainCamera->front * velocity;
		if (keyboardState->keys[GLFW_KEY_S]) mainCamera->position -= mainCamera->front * velocity;
		if (keyboardState->keys[GLFW_KEY_A]) mainCamera->position -= mainCamera->right * velocity;
		if (keyboardState->keys[GLFW_KEY_D]) mainCamera->position += mainCamera->right * velocity;

		mainCamera->updateCameraVectors();
	}
}