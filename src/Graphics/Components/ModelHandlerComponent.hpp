#pragma once

#include <memory>
#include "../Model.hpp"

struct ModelHandlerComponent
{
	Model* modelInstance;

	ModelHandlerComponent(Model* model) : modelInstance(model) {}
};