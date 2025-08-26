#pragma once
#include "../../Core/Components/ComponentManager.h"
#include "../Shader.h"

struct ShaderComponent : Component
{
	Shader* ShaderInstance;
	ShaderComponent(const char* vertexPath, const char* fragmentPath) : 
	ShaderInstance(new Shader(vertexPath, fragmentPath))
	{
	}
};