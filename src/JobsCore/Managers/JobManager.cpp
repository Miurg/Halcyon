#include "JobsCore/Managers/JobManager.hpp"

#include <cstdlib>
#include <iostream>

#include "JobsCore/JobRuntime.hpp"

JobManager::JobManager(JobRuntime& runtime) : _runtime(runtime), _mainThreadId(std::this_thread::get_id()) {}

JobManager::~JobManager()
{
	if (!_shuttingDown.load(std::memory_order_acquire) && _runtime.currentWorkerSlot() < 0)
	{
		shutdown();
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
