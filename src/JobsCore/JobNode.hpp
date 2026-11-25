#pragma once

#include <atomic>
#include <functional>

#include "JobsCore/JobTypes.hpp"

struct JobNode
{
	std::atomic<JobStatus> status{JobStatus::Pending};
	JobLane lane = JobLane::Worker;
	const char* name = nullptr;
	std::function<void()> work;

	[[nodiscard]] static bool isTerminal(JobStatus s) noexcept
	{
		return s == JobStatus::Completed || s == JobStatus::Cancelled;
	}

	void finish(JobStatus finalStatus) noexcept
	{
		status.store(finalStatus, std::memory_order_release);
		status.notify_all();
	}
};
