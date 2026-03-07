#pragma once
#include "../../Orhescyon/Systems/SystemCore.hpp"
#include "../../Orhescyon/GeneralManager.hpp"
#include "../../Platform/Window.hpp"
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
};