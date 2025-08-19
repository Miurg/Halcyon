#pragma once
#include "../../Core/Components/ComponentManager.h"
#include "../Shader.h"

struct ShaderComponent : Component
{
	Shader* ShaderInstance;
	ShaderComponent(Shader* shader) : ShaderInstance(shader) {}
};