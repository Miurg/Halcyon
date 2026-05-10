#pragma once
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "../../Platform/Window.hpp"
#include "../../Graphics/Systems/DeltaTimeSystem.hpp"
#include "../../Graphics/Systems/FrameBeginSystem.hpp"
#include "../../Graphics/Components/DeltaTimeComponent.hpp"
#include "../../Platform/Components/KeyboardStateComponent.hpp"
#include "../../Platform/Components/CursorPositionComponent.hpp"
#include "../../Graphics/Components/CameraComponent.hpp"
#include "../../Graphics/Components/GlobalTransformComponent.hpp"
#include "../Components/ControlComponent.hpp"

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