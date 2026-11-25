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
	JobRuntime(const JobRuntime&) = delete;
	JobRuntime& operator=(const JobRuntime&) = delete;
	JobRuntime(JobRuntime&&) = delete;
	JobRuntime& operator=(JobRuntime&&) = delete;

	explicit JobRuntime(std::size_t workerCount = defaultWorkerCount(),
	                    std::size_t backgroundWorkerCount = defaultBackgroundWorkerCount());
	~JobRuntime();

	static std::size_t defaultWorkerCount() noexcept;
	static std::size_t defaultBackgroundWorkerCount() noexcept;

	[[nodiscard]] std::size_t workerCount() const noexcept;
	[[nodiscard]] std::size_t backgroundWorkerCount() const noexcept;
	[[nodiscard]] std::size_t totalWorkerCount() const noexcept;

	[[nodiscard]] int currentWorkerIndex() const noexcept;
	[[nodiscard]] int currentBackgroundWorkerIndex() const noexcept;
	// Returns [0, workerCount) for frame workers, [workerCount, totalWorkerCount) for background workers, or -1.
	[[nodiscard]] int currentWorkerSlot() const noexcept;

	[[nodiscard]] tf::Executor& executor() noexcept;
	[[nodiscard]] tf::Executor& backgroundExecutor() noexcept;
	void waitForAll();

private:
	std::unique_ptr<tf::Executor> _executor;
	std::unique_ptr<tf::Executor> _backgroundExecutor;
};
