#include "JobsCore/JoltTaskflowAdapter.hpp"
#include "JobsCore/JobRuntime.hpp"

#include <Jolt/Core/FixedSizeFreeList.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <atomic>
#include <taskflow/taskflow.hpp>

struct JoltTaskflowAdapter::State
{
	struct TaskflowJob final : Job
	{
		using Job::Job;
		bool pooled = true;
	};

	JPH::FixedSizeFreeList<TaskflowJob> jobs;
	// The last wrapper may still notify after the adapter has finished waiting.
	std::shared_ptr<std::atomic<std::size_t>> pendingJobs = std::make_shared<std::atomic<std::size_t>>(0);
};

JoltTaskflowAdapter::JoltTaskflowAdapter(JobRuntime& runtime)
    : JPH::JobSystemWithBarrier(JPH::cMaxPhysicsBarriers), _runtime(runtime), _state(std::make_unique<State>())
{
	_state->jobs.Init(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsJobs);
}

JoltTaskflowAdapter::~JoltTaskflowAdapter()
{
	waitForQueuedJobs();
}

int JoltTaskflowAdapter::GetMaxConcurrency() const
{
	return static_cast<int>(_runtime.workerCount()) + 1;
}

JPH::JobHandle JoltTaskflowAdapter::CreateJob(const char* name, JPH::ColorArg color, const JobFunction& function,
                                              JPH::uint32 dependencyCount)
{
	const JPH::uint32 index = _state->jobs.ConstructObject(name, color, this, function, dependencyCount);
	State::TaskflowJob* job;
	if (index == JPH::FixedSizeFreeList<State::TaskflowJob>::cInvalidObjectIndex)
	{
		// Barrier execution can outpace queued wrappers, so exhausting the pool must not stall the worker.
		job = new State::TaskflowJob(name, color, this, function, dependencyCount);
		job->pooled = false;
	}
	else
	{
		job = &_state->jobs.Get(index);
	}

	JobHandle handle(job);
	if (dependencyCount == 0) QueueJob(job);
	return handle;
}

void JoltTaskflowAdapter::WaitForJobs(Barrier* barrier)
{
	JPH::JobSystemWithBarrier::WaitForJobs(barrier);
	// A finished barrier can still have Taskflow wrappers holding references to its jobs.
	waitForQueuedJobs();
}

void JoltTaskflowAdapter::QueueJob(Job* job)
{
	const auto pendingJobs = _state->pendingJobs;
	job->AddRef();
	pendingJobs->fetch_add(1, std::memory_order_relaxed);

	try
	{
		_runtime.executor().silent_async(
		    [job, pendingJobs]() noexcept
		    {
			    job->Execute();
			    job->Release();
			    if (pendingJobs->fetch_sub(1, std::memory_order_acq_rel) == 1) pendingJobs->notify_all();
		    });
	}
	catch (...)
	{
		job->Release();
		if (pendingJobs->fetch_sub(1, std::memory_order_acq_rel) == 1) pendingJobs->notify_all();
		throw;
	}
}

void JoltTaskflowAdapter::QueueJobs(Job** jobs, JPH::uint count)
{
	for (JPH::uint index = 0; index < count; ++index) QueueJob(jobs[index]);
}

void JoltTaskflowAdapter::FreeJob(Job* job)
{
	auto* taskflowJob = static_cast<State::TaskflowJob*>(job);
	if (taskflowJob->pooled)
	{
		_state->jobs.DestructObject(taskflowJob);
	}
	else
	{
		delete taskflowJob;
	}
}

void JoltTaskflowAdapter::waitForQueuedJobs()
{
	const auto pendingJobs = _state->pendingJobs;
	tf::Executor& executor = _runtime.executor();
	if (executor.this_worker_id() >= 0)
	{
		executor.corun_until([&pendingJobs]() { return pendingJobs->load(std::memory_order_acquire) == 0; });
	}
	else
	{
		for (std::size_t count = pendingJobs->load(std::memory_order_acquire); count != 0;
		     count = pendingJobs->load(std::memory_order_acquire))
		{
			pendingJobs->wait(count, std::memory_order_acquire);
		}
	}
}
