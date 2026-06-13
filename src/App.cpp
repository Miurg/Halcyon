#include "App.hpp"

#include <iostream>
#include <exception>

#include "Game/GameInit.hpp"
#include "GraphicsCore/GraphicsInit/GraphicsInit.hpp"
#include "MainLoop.hpp"
#include "PhysicsCore/PhysicsInit.hpp"
#include "PlatformCore/PlatformInit.hpp"

#include "DeletionQueueComponent.hpp"
#include "DeletionQueueContext.hpp"

App::App() : deletionQueue(&gm)
{
	Orhescyon::Entity dqEntity = gm.createEntity();
	gm.registerContext<DeletionQueueContext>(dqEntity);
	gm.addComponent<DeletionQueueComponent>(dqEntity, &deletionQueue);

	try
	{
		PhysicsInit::Run(gm);
		PlatformInit::Run(gm);
		GraphicsInit::Run(gm);
	}
	catch (const std::exception& e)
	{
		std::cerr << "ERROR::APP::INIT::Exception: " << e.what() << std::endl;
		return;
	}
}
App::~App() = default;

using Orhescyon::GeneralManager;

int App::run()
{
	try
	{
		MainLoop::startLoop(gm);
	}
	catch (const std::exception& e)
	{
		std::cerr << "ERROR::APP::MAINLOOP::Exception: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

App App::create()
{
	return App();
}

App& App::addStartUp(IStartUp& startUp)
{
	try
	{
		startUp.Run(gm);
	}
	catch (const std::exception& e)
	{
		std::cerr << "ERROR::APP::INIT::Exception: " << e.what() << std::endl;
		return *this;
	}
	return *this;
}