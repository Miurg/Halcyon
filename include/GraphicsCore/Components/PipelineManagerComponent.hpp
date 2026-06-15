#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Managers/PipelineManager.hpp"

struct HALCYON_API PipelineManagerComponent
{
	PipelineManager* pipelineManager;

	PipelineManagerComponent(PipelineManager* pipeline) : pipelineManager(pipeline) {}
};