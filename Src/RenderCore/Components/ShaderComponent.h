#pragma once
#include "../../Core/Components/ComponentManager.h"
#include "../Shader.h"

struct ShaderComponent : Component
{
	std::unique_ptr<Shader> ShaderInstance;
	ShaderComponent(const char* vertexPath, const char* fragmentPath)
	    : ShaderInstance(std::make_unique<Shader>(vertexPath, fragmentPath))
	{
	}
};