#pragma once

#include "HalcyonExport.hpp"
#include <cstddef>
#include <memory>

namespace tf
{
class Executor;
}

class HALCYON_API JobRuntime final
{
public:
	static std::size_t defaultWorkerCount() noexcept;

	explicit JobRuntime(std::size_t workerCount = defaultWorkerCount());
	~JobRuntime();

	JobRuntime(const JobRuntime&) = delete;
	JobRuntime& operator=(const JobRuntime&) = delete;
	JobRuntime(JobRuntime&&) = delete;
	JobRuntime& operator=(JobRuntime&&) = delete;

	[[nodiscard]] std::size_t workerCount() const noexcept;
	[[nodiscard]] tf::Executor& executor() noexcept;
	void waitForAll();

private:
	std::unique_ptr<tf::Executor> _executor;
};
