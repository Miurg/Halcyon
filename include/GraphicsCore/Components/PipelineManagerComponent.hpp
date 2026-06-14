#pragma once

#include "GraphicsCore/Managers/PipelineManager.hpp"

struct PipelineManagerComponent
{
	PipelineManager* pipelineManager;

	PipelineManagerComponent(PipelineManager* pipeline) : pipelineManager(pipeline) {}
};