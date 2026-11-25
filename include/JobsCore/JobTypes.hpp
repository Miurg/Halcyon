#pragma once

#include <cstdint>

#include "HalcyonExport.hpp"

enum class JobLane : uint8_t
{
	Worker = 0,
	Background = 1
};

enum class JobStatus : uint8_t
{
	Pending = 0,
	Running = 1,
	Completed = 2,
	Cancelled = 3
};

struct HALCYON_API JobDesc
{
	JobLane lane = JobLane::Worker;
	const char* name = nullptr;
};
