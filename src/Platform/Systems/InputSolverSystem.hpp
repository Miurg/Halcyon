#pragma once

#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "../Components/CursorPositionComponent.hpp"
#include "../Components/WindowSizeComponent.hpp"
#include "../Components/KeyboardStateComponent.hpp"
#include "../Components/MouseStateComponent.hpp"
#include "../Components/ScrollDeltaComponent.hpp"
#include "../Components/WindowComponent.hpp"
#include "../../Graphics/Systems/DeltaTimeSystem.hpp"
#include "../../Graphics/Systems/FrameBeginSystem.hpp"

using Orhescyon::GeneralManager;
class InputSolverSystem
    : public Orhescyon::SystemCore<InputSolverSystem, WindowComponent, CursorPositionComponent, WindowSizeComponent,
                        KeyboardStateComponent, MouseStateComponent, ScrollDeltaComponent>
{
public:
	std::vector<Entity> entities;
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	void onEntitySubscribed(Entity entity, GeneralManager& gm) override
	{
		entities.push_back(entity);
	}
	std::vector<std::type_index> getAfterSystems() override
	{
		return {typeid(DeltaTimeSystem)};
	}
	std::vector<std::type_index> getBeforeSystems() override
	{
		return {typeid(FrameBeginSystem)};
	}
	std::vector<std::type_index> getWriteComponents() override
	{
		return {typeid(KeyboardStateComponent), typeid(MouseStateComponent), typeid(CursorPositionComponent),
		        typeid(ScrollDeltaComponent), typeid(WindowSizeComponent)};
	}
};