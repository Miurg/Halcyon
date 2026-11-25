#include "JobsCore/JobRuntime.hpp"

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <thread>
#include <taskflow/taskflow.hpp>

#include "PlatformCore/Platform.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

namespace
{
// Configures per-worker OS priority and profiler thread names upon entering the work-stealing loop.
class RuntimeWorkerInterface final : public tf::WorkerInterface
{
public:
	RuntimeWorkerInterface(const char* namePrefix, bool lowerPriority)
	    : _namePrefix(namePrefix), _lowerPriority(lowerPriority)
	{
	}

	void scheduler_prologue(tf::Worker& worker) override
	{
		if (_lowerPriority) Platform::lowerCurrentThreadPriority();

#ifdef TRACY_ENABLE
		thread_local char threadName[32];
		std::snprintf(threadName, sizeof(threadName), "%s %zu", _namePrefix, worker.id());
		tracy::SetThreadName(threadName);
#else
		(void)worker;
#endif
	}

	void scheduler_epilogue(tf::Worker& worker, std::exception_ptr ptr) override
	{
		(void)worker;
		if (ptr)
		{
			std::cerr << "JobRuntime: unhandled exception escaped worker loop" << std::endl;
			std::abort();
		}
	}

private:
	const char* _namePrefix;
	bool _lowerPriority;
};
} // namespace

JobRuntime::JobRuntime(std::size_t workerCount, std::size_t backgroundWorkerCount)
{
	if (workerCount == 0 || backgroundWorkerCount == 0)
	{
		std::cerr << "JobRuntime: worker counts must be at least 1" << std::endl;
		std::abort();
	}

	_executor =
	    std::make_unique<tf::Executor>(workerCount, std::make_shared<RuntimeWorkerInterface>("JobWorker", false));
	_backgroundExecutor = std::make_unique<tf::Executor>(
	    backgroundWorkerCount, std::make_shared<RuntimeWorkerInterface>("JobBackground", true));
}

JobRuntime::~JobRuntime()
{
	if (_executor && _backgroundExecutor && _executor->this_worker_id() < 0 && _backgroundExecutor->this_worker_id() < 0)
	{
		waitForAll();
	}
}

std::size_t JobRuntime::defaultWorkerCount() noexcept
{
	const unsigned int hardwareThreads = std::thread::hardware_concurrency();
	return hardwareThreads > 2 ? hardwareThreads - 2 : 1;
}

std::size_t JobRuntime::defaultBackgroundWorkerCount() noexcept
{
	const unsigned int hardwareThreads = std::thread::hardware_concurrency();
	return hardwareThreads >= 8 ? 2 : 1;
}

std::size_t JobRuntime::workerCount() const noexcept
{
	return _executor->num_workers();
}

std::size_t JobRuntime::backgroundWorkerCount() const noexcept
{
	return _backgroundExecutor->num_workers();
}

std::size_t JobRuntime::totalWorkerCount() const noexcept
{
	return _executor->num_workers() + _backgroundExecutor->num_workers();
}

int JobRuntime::currentWorkerIndex() const noexcept
{
	return _executor->this_worker_id();
}

int JobRuntime::currentBackgroundWorkerIndex() const noexcept
{
	return _backgroundExecutor->this_worker_id();
}

int JobRuntime::currentWorkerSlot() const noexcept
{
	const int workerId = _executor->this_worker_id();
	if (workerId >= 0) return workerId;

	const int backgroundId = _backgroundExecutor->this_worker_id();
	if (backgroundId >= 0) return static_cast<int>(_executor->num_workers()) + backgroundId;

	return -1;
}

tf::Executor& JobRuntime::executor() noexcept
{
	return *_executor;
}

tf::Executor& JobRuntime::backgroundExecutor() noexcept
{
	return *_backgroundExecutor;
}

void JobRuntime::waitForAll()
{
	if (_executor->this_worker_id() >= 0 || _backgroundExecutor->this_worker_id() >= 0)
	{
		std::cerr << "JobRuntime: waitForAll must be called outside its workers" << std::endl;
		std::abort();
	}

	// Loop until both pools stay idle in case tasks in one pool enqueue continuations onto the other.
	do
	{
		_backgroundExecutor->wait_for_all();
		_executor->wait_for_all();
	} while (_backgroundExecutor->num_topologies() != 0 || _executor->num_topologies() != 0);
}
