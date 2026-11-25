#pragma once

#include <atomic>
#include <cstddef>
#include <thread>

#include "HalcyonExport.hpp"

class JobRuntime;

class HALCYON_API JobManager final
{
public:
	explicit JobManager(JobRuntime& runtime);
	~JobManager();

	JobManager(const JobManager&) = delete;
	JobManager& operator=(const JobManager&) = delete;
	JobManager(JobManager&&) = delete;
	JobManager& operator=(JobManager&&) = delete;

	[[nodiscard]] std::size_t workerCount() const noexcept;
	[[nodiscard]] std::size_t backgroundWorkerCount() const noexcept;
	[[nodiscard]] std::size_t totalWorkerCount() const noexcept;

	[[nodiscard]] int currentWorkerIndex() const noexcept;
	[[nodiscard]] int currentBackgroundWorkerIndex() const noexcept;
	[[nodiscard]] int currentWorkerSlot() const noexcept;

	[[nodiscard]] bool isMainThread() const noexcept;
	[[nodiscard]] bool isWorkerThread() const noexcept;
	[[nodiscard]] bool isShuttingDown() const noexcept;

	void shutdown();

private:
	JobRuntime& _runtime;
	std::thread::id _mainThreadId;
	std::atomic<bool> _shuttingDown{false};
};
