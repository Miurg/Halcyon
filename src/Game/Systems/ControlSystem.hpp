#pragma once
#include <GLFW/glfw3.h>
#include "../../Graphics/GraphicsContexts.hpp"
#include "../../Platform/PlatformContexts.hpp"
#include "../../Core/Systems/SystemContextual.hpp"
#include "../../Core/GeneralManager.hpp"
#include "../../Graphics/Components/CameraComponent.hpp"
#include "../../Platform/Components/WindowComponent.hpp"
#include "../../Platform/Components/KeyboardStateComponent.hpp"
#include "../../Platform/Components/CursorPositionComponent.hpp"

class ControlSystem : public SystemContextual<ControlSystem>
{
private:
	bool cursorDisable = true;
	bool cursorDisableKey = false;
	double lastMousePositionX = 0.0;
	double lastMousePositionY = 0.0;

	void cursorDisableToggle(Window* window);

public:
	void update(float deltaTime, GeneralManager& gm) override;
};