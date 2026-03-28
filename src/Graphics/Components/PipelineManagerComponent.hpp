#pragma once

#include "../Managers/PipelineManager.hpp"

struct PipelineManagerComponent
{
	PipelineManager* pipelineManager;

	PipelineManagerComponent(PipelineManager* pipeline) : pipelineManager(pipeline) {}
};