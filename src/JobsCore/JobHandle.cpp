#include "JobsCore/JobHandle.hpp"

#include <utility>

#include "JobsCore/JobNode.hpp"

JobHandle::JobHandle(std::shared_ptr<JobNode> node) noexcept : _node(std::move(node)) {}

bool JobHandle::valid() const noexcept
{
	return static_cast<bool>(_node);
}

bool JobHandle::isDone() const noexcept
{
	return !_node || JobNode::isTerminal(_node->status.load(std::memory_order_acquire));
}

JobStatus JobHandle::status() const noexcept
{
	if (!_node) return JobStatus::Completed;
	return _node->status.load(std::memory_order_acquire);
}
