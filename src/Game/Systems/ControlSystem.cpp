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
	if (cursorDisable)
	{
		window->setCursorMode(GLFW_CURSOR_NORMAL);
		cursorDisable = !cursorDisable;
	}
	else
	{
		window->setCursorMode(GLFW_CURSOR_DISABLED);
		int width, height;
		window->getWindowSize(&width, &height);
		window->setCursorPos(width / 2.0, height / 2.0);
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
		windowInstance->windowInstance->setShouldClose(true);
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

		// Prevent huge jumps the first frame or when toggling
		if (xoffset > 50.0f || xoffset < -50.0f || yoffset > 50.0f || yoffset < -50.0f)
		{
			xoffset = 0.0f;
			yoffset = 0.0f;
		}

		xoffset *= mainCameraControl->mouseSensitivity;
		yoffset *= mainCameraControl->mouseSensitivity;
		mainCameraTransform->rotateGlobal(glm::radians(-xoffset), glm::vec3(0.0f, 1.0f, 0.0f));
		mainCameraTransform->rotateLocal(glm::radians(-yoffset), glm::vec3(1.0f, 0.0f, 0.0f));

		int width, height;
		windowInstance->windowInstance->getWindowSize(&width, &height);
		double centerX = width / 2.0;
		double centerY = height / 2.0;

		windowInstance->windowInstance->setCursorPos(centerX, centerY);
		lastMousePositionX = centerX;
		lastMousePositionY = centerY;
		cursorPositionState->mousePositionX = centerX;
		cursorPositionState->mousePositionY = centerY;

		//=== Keyboard ===
		float velocity = mainCameraControl->movementSpeed * deltaTime;
		if (keyboardState->keys[GLFW_KEY_W]) mainCameraTransform->globalPosition += mainCameraTransform->front * velocity;
		if (keyboardState->keys[GLFW_KEY_S]) mainCameraTransform->globalPosition -= mainCameraTransform->front * velocity;
		if (keyboardState->keys[GLFW_KEY_A]) mainCameraTransform->globalPosition -= mainCameraTransform->right * velocity;
		if (keyboardState->keys[GLFW_KEY_D]) mainCameraTransform->globalPosition += mainCameraTransform->right * velocity;
	}
}
