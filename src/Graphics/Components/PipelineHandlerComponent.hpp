#pragma once

#include <memory>
#include "../PipelineHandler.hpp"

struct PipelineHandlerComponent
{
	std::unique_ptr<PipelineHandler> pipelineHandlerComponent;

	PipelineHandlerComponent() : pipelineHandlerComponent(std::make_unique<PipelineHandler>()) {}

	~PipelineHandlerComponent() = default;

	// Disable copy semantics
	PipelineHandlerComponent(const PipelineHandlerComponent&) = delete;
	PipelineHandlerComponent& operator=(const PipelineHandlerComponent&) = delete;

	// Enable move semantics
	PipelineHandlerComponent(PipelineHandlerComponent&&) = default;
	PipelineHandlerComponent& operator=(PipelineHandlerComponent&&) = default;
};