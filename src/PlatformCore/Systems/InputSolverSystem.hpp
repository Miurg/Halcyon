#pragma once

#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "PlatformCore/Components/CursorPositionComponent.hpp"
#include "PlatformCore/Components/WindowSizeComponent.hpp"
#include "PlatformCore/Components/KeyboardStateComponent.hpp"
#include "PlatformCore/Components/MouseStateComponent.hpp"
#include "PlatformCore/Components/ScrollDeltaComponent.hpp"
#include "PlatformCore/Components/WindowComponent.hpp"
#include "../../GraphicsCore/Systems/DeltaTimeSystem.hpp"
#include "../../GraphicsCore/Systems/FrameBeginSystem.hpp"

using Orhescyon::GeneralManager;
class InputSolverSystem
    : public Orhescyon::SystemCore<InputSolverSystem, WindowComponent, CursorPositionComponent, WindowSizeComponent,
                        KeyboardStateComponent, MouseStateComponent, ScrollDeltaComponent>
{
public:
	std::vector<Orhescyon::Entity> entities;
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	void onEntitySubscribed(Orhescyon::Entity entity, GeneralManager& gm) override
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