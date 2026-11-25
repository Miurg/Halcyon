#pragma once

#include "HalcyonExport.hpp"
#include <Orhescyon/Jobs/IJobSystem.hpp>

class JobManager;

class HALCYON_API OrhescyonTaskflowAdapter final : public Orhescyon::IJobSystem
{
public:
	explicit OrhescyonTaskflowAdapter(JobManager& jobManager);

	void parallelFor(std::size_t count, const std::function<void(std::size_t)>& body) override;

private:
	JobManager& _jobManager;
};
