#include "App.hpp"

#include <iostream>

#include "Game/GameInit.hpp"
#include "Graphics/GraphicsInit/GraphicsInit.hpp"
#include "Cleanup.hpp"
#include "MainLoop.hpp"
#include <exception>
#include <Orhescyon/GeneralManager.hpp>
#include "PhysicsCore/PhysicsInit.hpp"
#include "Platform/PlatformInit.hpp"

App::App() {}

using Orhescyon::GeneralManager;

int App::run()
{
	GeneralManager gm;
	try
	{
		PhysicsInit::Run(gm);
		PlatformInit::Run(gm);
		GraphicsInit::Run(gm);
		GameInit::Run(gm);
	}
	catch (const std::exception& e)
	{
		std::cerr << "ERROR::APP::INIT::Exception: " << e.what() << std::endl;
		Cleanup::cleanup(gm);
		return EXIT_FAILURE;
	}

	try
	{
		MainLoop::startLoop(gm);
	}
	catch (const std::exception& e)
	{
		std::cerr << "ERROR::APP::MAINLOOP::Exception: " << e.what() << std::endl;
		Cleanup::cleanup(gm);
		return EXIT_FAILURE;
	}
	Cleanup::cleanup(gm);
	return EXIT_SUCCESS;
}
