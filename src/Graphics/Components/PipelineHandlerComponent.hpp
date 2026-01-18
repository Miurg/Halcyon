#pragma once

#include "../PipelineHandler.hpp"

struct PipelineHandlerComponent
{
	PipelineHandler* pipelineHandler;

	PipelineHandlerComponent(PipelineHandler* pipeline) : pipelineHandler(pipeline) {}
};