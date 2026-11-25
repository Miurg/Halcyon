#include "JobsInit.hpp"

#include <iostream>

#include "DeletionQueueComponent.hpp"
#include "DeletionQueueContext.hpp"
#include "JobsCore/Components/JobManagerComponent.hpp"
#include "JobsCore/JobsContexts.hpp"
#include "JobsCore/Managers/JobManager.hpp"
#include "JobsCore/OrhescyonTaskflowAdapter.hpp"

void JobsInit::Run(Orhescyon::GeneralManager& gm, JobRuntime& jobRuntime)
{
#ifdef _DEBUG
	std::cout << "JOBSINIT::RUN::Start init" << std::endl;
#endif //_DEBUG

	initJobs(gm, jobRuntime);

#ifdef _DEBUG
	std::cout << "JOBSINIT::RUN::Succes!" << std::endl;
#endif //_DEBUG
}

void JobsInit::initJobs(Orhescyon::GeneralManager& gm, JobRuntime& jobRuntime)
{
	DeletionQueue* dq = gm.getContextComponent<DeletionQueueContext, DeletionQueueComponent>()->queue;

	Orhescyon::Entity jobManagerEntity = gm.createEntity();
	JobManager* jobManager = new JobManager(jobRuntime);
	gm.addComponentImmediate<JobManagerComponent>(jobManagerEntity, jobManager);
	gm.registerContext<JobManagerContext>(jobManagerEntity);
	dq->push_function([jobManager]() { delete jobManager; });

	OrhescyonTaskflowAdapter* ecsJobs = new OrhescyonTaskflowAdapter(*jobManager);
	gm.setJobSystem(ecsJobs);
	dq->push_function([ecsJobs]() { delete ecsJobs; });
}
