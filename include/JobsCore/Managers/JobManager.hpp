#pragma once

#include <atomic>
#include <concepts>
#include <cstddef>
#include <functional>
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

	template <typename Func>
	   requires(std::invocable<Func&, std::size_t, std::size_t> || std::invocable<Func&, std::size_t>)
	void parallelFor(std::size_t count, Func&& body, std::size_t grainSize = 0, const char* name = nullptr)
	{
		if (count == 0) return;

		if constexpr (std::invocable<Func&, std::size_t, std::size_t>)
		{
			parallelForRange(count, grainSize, [&body](std::size_t begin, std::size_t end) { body(begin, end); }, name);
		}
		else
		{
			parallelForRange(
			    count, grainSize,
			    [&body](std::size_t begin, std::size_t end)
			    {
				    for (std::size_t index = begin; index < end; ++index) body(index);
			    },
			    name);
		}
	}

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
	void parallelForRange(std::size_t count, std::size_t grainSize,
	                      const std::function<void(std::size_t begin, std::size_t end)>& body,
	                      const char* name = nullptr);

	JobRuntime& _runtime;
	std::thread::id _mainThreadId;
	std::atomic<bool> _shuttingDown{false};
};
