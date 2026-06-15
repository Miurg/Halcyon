#pragma once

#include <Orhescyon/GeneralManager.hpp>

#include "DeletionQueue.hpp"
#include "IStartUp.hpp"

class App
{
public:
	App();
	~App();

	App(App&&) = default;
	App& operator=(App&&) = default;

	App(const App&) = delete;
	App& operator=(const App&) = delete;

	int run();
	static App create();
	App& addStartUp(IStartUp& startUp);


private:
	Orhescyon::GeneralManager gm;
	DeletionQueue deletionQueue;
};