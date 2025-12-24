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
	void processEntity(Entity entity, GeneralManager& gm, float deltaTime);
};