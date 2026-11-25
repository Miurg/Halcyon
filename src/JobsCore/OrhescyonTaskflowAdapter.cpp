#include "JobsCore/OrhescyonTaskflowAdapter.hpp"

#include "JobsCore/Managers/JobManager.hpp"

OrhescyonTaskflowAdapter::OrhescyonTaskflowAdapter(JobManager& jobManager) : _jobManager(jobManager) {}

void OrhescyonTaskflowAdapter::parallelFor(std::size_t count, const std::function<void(std::size_t)>& body)
{
	_jobManager.parallelFor(count, body, 0, "Orhescyon::parallelFor");
}
