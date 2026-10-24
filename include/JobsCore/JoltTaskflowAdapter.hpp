#pragma once

#include "HalcyonExport.hpp"
#include <Jolt/Jolt.h>
#include <Jolt/Core/JobSystemWithBarrier.h>
#include <memory>

class JobRuntime;

class HALCYON_API JoltTaskflowAdapter final : public JPH::JobSystemWithBarrier
{
public:
	explicit JoltTaskflowAdapter(JobRuntime& runtime);
	~JoltTaskflowAdapter() override;

	int GetMaxConcurrency() const override;
	JobHandle CreateJob(const char* name, JPH::ColorArg color, const JobFunction& function,
	                    JPH::uint32 dependencyCount = 0) override;
	void WaitForJobs(Barrier* barrier) override;

protected:
	void QueueJob(Job* job) override;
	void QueueJobs(Job** jobs, JPH::uint count) override;
	void FreeJob(Job* job) override;

private:
	void waitForQueuedJobs();

	struct State;
	JobRuntime& _runtime;
	std::unique_ptr<State> _state;
};