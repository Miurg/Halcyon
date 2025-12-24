#pragma once

#include <memory>
#include "../Model.hpp"

struct ModelHandlerComponent
{
	std::unique_ptr<Model> modelHandlerComponent;

	ModelHandlerComponent() : modelHandlerComponent(std::make_unique<Model>()) {}

	~ModelHandlerComponent() = default;

	// Disable copy semantics
	ModelHandlerComponent(const ModelHandlerComponent&) = delete;
	ModelHandlerComponent& operator=(const ModelHandlerComponent&) = delete;

	// Enable move semantics
	ModelHandlerComponent(ModelHandlerComponent&&) = default;
	ModelHandlerComponent& operator=(ModelHandlerComponent&&) = default;
};