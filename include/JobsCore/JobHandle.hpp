#pragma once

#include <memory>

#include "HalcyonExport.hpp"
#include "JobsCore/JobTypes.hpp"

struct JobNode;
class JobManager;

class HALCYON_API JobHandle final
{
public:
	JobHandle() = default;

	[[nodiscard]] bool valid() const noexcept;
	[[nodiscard]] bool isDone() const noexcept;
	[[nodiscard]] JobStatus status() const noexcept;

private:
	friend class JobManager;

	explicit JobHandle(std::shared_ptr<JobNode> node) noexcept;

	std::shared_ptr<JobNode> _node;
};
