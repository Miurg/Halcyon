#pragma once
#include "../../Core/Systems/SystemCore.hpp"
#include "../../Core/GeneralManager.hpp"
#include "../../Platform/Window.hpp"
class ControlSystem : public SystemCore<ControlSystem>
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
};