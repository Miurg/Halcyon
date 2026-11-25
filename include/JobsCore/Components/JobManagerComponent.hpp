#pragma once

#include "HalcyonExport.hpp"
#include "JobsCore/Managers/JobManager.hpp"

struct HALCYON_API JobManagerComponent
{
	JobManager* jobManager = nullptr;
};
