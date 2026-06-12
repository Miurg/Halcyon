#pragma once

#include <Orhescyon/GeneralManager.hpp>

#include "DeletionQueue.hpp"

class App
{
public:
	App();
	~App();
	int run();

private:
	Orhescyon::GeneralManager gm;
	DeletionQueue deletionQueue;
};
