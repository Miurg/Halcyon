#pragma once

#include "../../Core/Systems/SystemCore.hpp"
#include "../Components/CursorPositionComponent.hpp"
#include "../Components/WindowSizeComponent.hpp"
#include "../Components/KeyboardStateComponent.hpp"
#include "../Components/MouseStateComponent.hpp"
#include "../Components/ScrollDeltaComponent.hpp"
#include "../Components/WindowComponent.hpp"

class InputSolverSystem
    : public SystemCore<InputSolverSystem, WindowComponent, CursorPositionComponent, WindowSizeComponent,
                              KeyboardStateComponent, MouseStateComponent, ScrollDeltaComponent>
{
public:
	std::vector<Entity> entities;
	void update(float deltaTime, GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	void onEntitySubscribed(Entity entity, GeneralManager& gm) override
	{
		entities.push_back(entity);
	}
};