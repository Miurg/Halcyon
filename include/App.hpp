#pragma once

#include "HalcyonExport.hpp"
#include <Orhescyon/GeneralManager.hpp>

#include "DeletionQueue.hpp"
#include "IStartUp.hpp"
#include "JobsCore/JobRuntime.hpp"

class HALCYON_API App
{
public:
	explicit App(std::size_t workerCount = JobRuntime::defaultWorkerCount());
	~App();

	App(App&&) = delete;
	App& operator=(App&&) = delete;

	App(const App&) = delete;
	App& operator=(const App&) = delete;

	int run();
	static App create(std::size_t workerCount = JobRuntime::defaultWorkerCount());
	App& addStartUp(IStartUp& startUp);

private:
	JobRuntime jobRuntime;
	Orhescyon::GeneralManager gm;
	DeletionQueue deletionQueue;
};
