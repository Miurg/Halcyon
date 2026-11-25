#include "JobsCore/Managers/JobManager.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <taskflow/taskflow.hpp>

#include "JobsCore/JobRuntime.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

JobManager::JobManager(JobRuntime& runtime) : _runtime(runtime), _mainThreadId(std::this_thread::get_id()) {}

JobManager::~JobManager()
{
	if (!_shuttingDown.load(std::memory_order_acquire) && _runtime.currentWorkerSlot() < 0)
	{
		shutdown();
	}
}

void JobManager::parallelForRange(std::size_t count, std::size_t grainSize,
                                  const std::function<void(std::size_t begin, std::size_t end)>& body, const char* name)
{
	if (count == 0) return;

	const std::size_t maxParticipants = _runtime.workerCount() + 1;
	const std::size_t targetChunks = maxParticipants * 4;
	const std::size_t grain =
	    grainSize > 0 ? grainSize : std::max<std::size_t>(1, (count + targetChunks - 1) / targetChunks);

	if (count <= grain || _shuttingDown.load(std::memory_order_acquire))
	{
#ifdef TRACY_ENABLE
		ZoneTransientN(zone, name ? name : "JobManager::parallelFor", true);
#else
		(void)name;
#endif
		body(0, count);
		return;
	}

	const std::size_t chunkCount = (count + grain - 1) / grain;
	const std::size_t participantCount = std::min(chunkCount, maxParticipants);
	const std::size_t asyncTaskCount = participantCount - 1;

	std::atomic<std::size_t> nextChunk{0};
	// Heap-allocated because the last worker may still call notify_all() after the caller observes zero and returns.
	auto remainingTasks = std::make_shared<std::atomic<std::size_t>>(asyncTaskCount);

	auto runChunks = [count, grain, chunkCount, &nextChunk, &body, name]()
	{
#ifdef TRACY_ENABLE
		ZoneTransientN(zone, name ? name : "JobManager::parallelFor", true);
#else
		(void)name;
#endif
		for (;;)
		{
			const std::size_t chunkIndex = nextChunk.fetch_add(1, std::memory_order_relaxed);
			if (chunkIndex >= chunkCount) break;

			const std::size_t begin = chunkIndex * grain;
			const std::size_t end = begin + std::min(grain, count - begin);
			body(begin, end);
		}
	};

	tf::Executor& workerExecutor = _runtime.executor();
	for (std::size_t taskIndex = 0; taskIndex < asyncTaskCount; ++taskIndex)
	{
		workerExecutor.silent_async(
		    [&runChunks, remainingTasks]()
		    {
			    runChunks();
			    if (remainingTasks->fetch_sub(1, std::memory_order_acq_rel) == 1)
			    {
				    remainingTasks->notify_all();
			    }
		    });
	}

	// Participate on the calling thread as well so small batches make immediate progress without wakeup stalls.
	runChunks();

	if (workerExecutor.this_worker_id() >= 0)
	{
		workerExecutor.corun_until([&remainingTasks]() { return remainingTasks->load(std::memory_order_acquire) == 0; });
	}
	else if (tf::Executor& bgExecutor = _runtime.backgroundExecutor(); bgExecutor.this_worker_id() >= 0)
	{
		bgExecutor.corun_until([&remainingTasks]() { return remainingTasks->load(std::memory_order_acquire) == 0; });
	}
	else
	{
		for (std::size_t pending = remainingTasks->load(std::memory_order_acquire); pending != 0;
		     pending = remainingTasks->load(std::memory_order_acquire))
		{
			remainingTasks->wait(pending, std::memory_order_acquire);
		}
	}
}

std::size_t JobManager::workerCount() const noexcept
{
	return _runtime.workerCount();
}

std::size_t JobManager::backgroundWorkerCount() const noexcept
{
	return _runtime.backgroundWorkerCount();
}

std::size_t JobManager::totalWorkerCount() const noexcept
{
	return _runtime.totalWorkerCount();
}

int JobManager::currentWorkerIndex() const noexcept
{
	return _runtime.currentWorkerIndex();
}

int JobManager::currentBackgroundWorkerIndex() const noexcept
{
	return _runtime.currentBackgroundWorkerIndex();
}

int JobManager::currentWorkerSlot() const noexcept
{
	return _runtime.currentWorkerSlot();
}

bool JobManager::isMainThread() const noexcept
{
	return std::this_thread::get_id() == _mainThreadId;
}

bool JobManager::isWorkerThread() const noexcept
{
	return _runtime.currentWorkerSlot() >= 0;
}

bool JobManager::isShuttingDown() const noexcept
{
	return _shuttingDown.load(std::memory_order_acquire);
}

void JobManager::shutdown()
{
	if (_runtime.currentWorkerSlot() >= 0)
	{
		std::cerr << "JobManager: shutdown must be called outside worker threads" << std::endl;
		std::abort();
	}

	_shuttingDown.store(true, std::memory_order_release);
	_runtime.waitForAll();
}
