#pragma once

#include "HalcyonExport.hpp"
#include <Orhescyon/Jobs/IJobSystem.hpp>

class JobRuntime;

class HALCYON_API OrhescyonTaskflowAdapter final : public Orhescyon::IJobSystem
{
public:
	explicit OrhescyonTaskflowAdapter(JobRuntime& runtime);

	void parallelFor(std::size_t count, const std::function<void(std::size_t)>& body) override;

private:
	JobRuntime& _runtime;
};
