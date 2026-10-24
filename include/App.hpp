#pragma once

#include "HalcyonExport.hpp"
#include <Orhescyon/GeneralManager.hpp>

#include "DeletionQueue.hpp"
#include "IStartUp.hpp"
#include "JobsCore/JobRuntime.hpp"
#include "JobsCore/OrhescyonTaskflowAdapter.hpp"

class HALCYON_API App
{
public:
	explicit App(std::size_t workerCount = JobRuntime::defaultWorkerCount());
	~App();

	App(App&&) = default;
	App& operator=(App&&) = default;

	App(const App&) = delete;
	App& operator=(const App&) = delete;

	int run();
	static App create(std::size_t workerCount = JobRuntime::defaultWorkerCount());
	App& addStartUp(IStartUp& startUp);

private:
	JobRuntime jobRuntime;
	OrhescyonTaskflowAdapter ecsJobs;
	Orhescyon::GeneralManager gm;
	DeletionQueue deletionQueue;
};
