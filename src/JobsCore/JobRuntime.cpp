#include "JobsCore/JobRuntime.hpp"

#include <stdexcept>
#include <thread>
#include <taskflow/taskflow.hpp>

std::size_t JobRuntime::defaultWorkerCount() noexcept
{
	const unsigned int hardwareThreads = std::thread::hardware_concurrency();
	return hardwareThreads > 2 ? hardwareThreads - 2 : 1;
}

JobRuntime::JobRuntime(std::size_t workerCount)
{
	if (workerCount == 0) throw std::invalid_argument("JobRuntime: workerCount must be at least 1");
	_executor = std::make_unique<tf::Executor>(workerCount);
}

JobRuntime::~JobRuntime() = default;

std::size_t JobRuntime::workerCount() const noexcept
{
	return _executor->num_workers();
}

tf::Executor& JobRuntime::executor() noexcept
{
	return *_executor;
}

void JobRuntime::waitForAll()
{
	if (_executor->this_worker_id() >= 0)
	{
		throw std::logic_error("JobRuntime: waitForAll must be called outside its workers");
	}
	_executor->wait_for_all();
}
