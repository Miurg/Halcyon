#include "ControlSystem.hpp"
#include "../../GraphicsCore/Components/LocalTransformComponent.hpp"
#include "../../GraphicsCore/Components/CameraComponent.hpp"
#include "../../PlatformCore/Components/KeyboardStateComponent.hpp"
#include "../../PlatformCore/Components/CursorPositionComponent.hpp"
#include <GLFW/glfw3.h>
#include "../../GraphicsCore/GraphicsContexts.hpp"
#include "../../PlatformCore/PlatformContexts.hpp"
#include "../../GraphicsCore/Components/DeltaTimeComponent.hpp"
#include "../Components/ControlComponent.hpp"
#include "../../PlatformCore/Components/WindowComponent.hpp"
#include "../../PlatformCore/Window.hpp"
#include "../../GraphicsCore/Components/GlobalTransformComponent.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

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

void ControlSystem::update(GeneralManager& gm)
{
#ifdef TRACY_ENABLE
	ZoneScopedN("ControlSystem");
#endif


	float deltaTime = gm.getContextComponent<DeltaTimeContext, DeltaTimeComponent>()->deltaTime;
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
		if (keyboardState->keys[GLFW_KEY_W]) mainCameraTransform->moveGlobalPosition( mainCameraTransform->getFront() * velocity);
		if (keyboardState->keys[GLFW_KEY_S]) mainCameraTransform->moveGlobalPosition(-mainCameraTransform->getFront() * velocity);
		if (keyboardState->keys[GLFW_KEY_A]) mainCameraTransform->moveGlobalPosition(-mainCameraTransform->getRight() * velocity);
		if (keyboardState->keys[GLFW_KEY_D]) mainCameraTransform->moveGlobalPosition( mainCameraTransform->getRight() * velocity);
	}
}
