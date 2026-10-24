#include "JobsCore/OrhescyonTaskflowAdapter.hpp"
#include "JobsCore/JobRuntime.hpp"

#include <taskflow/taskflow.hpp>
#include <taskflow/algorithm/for_each.hpp>

OrhescyonTaskflowAdapter::OrhescyonTaskflowAdapter(JobRuntime& runtime) : _runtime(runtime) {}

void OrhescyonTaskflowAdapter::parallelFor(std::size_t count, const std::function<void(std::size_t)>& body)
{
	if (count == 0) return;
	if (count == 1)
	{
		body(0);
		return;
	}

	tf::Taskflow taskflow;
	taskflow.for_each_index(std::size_t{0}, count, std::size_t{1}, [&body](std::size_t index) { body(index); });

	tf::Executor& executor = _runtime.executor();
	if (executor.this_worker_id() >= 0)
	{
		executor.corun(taskflow);
	}
	else
	{
		executor.run(taskflow).get();
	}
}
