#include "App.hpp"

#include <iostream>
#include <exception>

#include "GraphicsCore/GraphicsInit/GraphicsInit.hpp"
#include "MainLoop.hpp"
#include "PhysicsCore/PhysicsInit.hpp"
#include "PlatformCore/PlatformInit.hpp"

#include "DeletionQueueComponent.hpp"
#include "DeletionQueueContext.hpp"
#include "AudioCore/AudioInit.hpp"

App::App(std::size_t workerCount) : jobRuntime(workerCount), ecsJobs(jobRuntime), gm(&ecsJobs), deletionQueue(&gm)
{
	Orhescyon::Entity dqEntity = gm.createEntity();
	gm.registerContext<DeletionQueueContext>(dqEntity);
	gm.addComponentImmediate<DeletionQueueComponent>(dqEntity, &deletionQueue);

	PhysicsInit::Run(gm, jobRuntime);
	PlatformInit::Run(gm);
	GraphicsInit::Run(gm);
	AudioInit::Run(gm);
}

App::~App()
{
	jobRuntime.waitForAll();
}

using Orhescyon::GeneralManager;

int App::run()
{
	MainLoop::startLoop(gm);
	return EXIT_SUCCESS;
}

App App::create(std::size_t workerCount)
{
	return App(workerCount);
}

App& App::addStartUp(IStartUp& startUp)
{
	startUp.Run(gm);
	return *this;
}