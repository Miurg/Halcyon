#pragma once
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>

#include "../Components/ControlComponent.hpp"
#include <GraphicsCore/Systems/FrameBeginSystem.hpp>
#include <GraphicsCore/Systems/DeltaTimeSystem.hpp>

#include <PlatformCore/Components/KeyboardStateComponent.hpp>
#include <PlatformCore/Components/CursorPositionComponent.hpp>
#include <GraphicsCore/Components/DeltaTimeComponent.hpp>
#include <GraphicsCore/Components/GlobalTransformComponent.hpp>
#include <GraphicsCore/Components/CameraComponent.hpp>
#include <PlatformCore/Components/WindowComponent.hpp>

using Orhescyon::GeneralManager;

class ControlSystem : public Orhescyon::SystemCore<ControlSystem>
{
private:
	bool cursorDisable = true;
	bool cursorDisableKey = false;
	double lastMousePositionX = 0.0;
	double lastMousePositionY = 0.0;

	void cursorDisableToggle(Window* window);

public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override
	{
		std::cout << "ControlSystem registered!" << std::endl;
	};
	void onShutdown(GeneralManager& gm) override
	{
		std::cout << "ControlSystem shutdown!" << std::endl;
	};
	std::vector<std::type_index> getAfterSystems() override
	{
		return {typeid(DeltaTimeSystem)};
	}
	std::vector<std::type_index> getBeforeSystems() override
	{
		return {typeid(FrameBeginSystem)};
	}
	std::vector<std::type_index> getReadComponents() override
	{
		return {typeid(DeltaTimeComponent), typeid(KeyboardStateComponent), typeid(CameraComponent),
		        typeid(ControlComponent)};
	}
	std::vector<std::type_index> getWriteComponents() override
	{
		return {typeid(GlobalTransformComponent), typeid(CursorPositionComponent)};
	}
};
