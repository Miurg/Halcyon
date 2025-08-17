#pragma once
#include "../../Core/Components/ComponentManager.h"

class MeshAsset;
class MaterialAsset;

struct ShaderComponent : Component
{
	Shader* ShaderInstance;
	ShaderComponent(Shader* shader) : ShaderInstance(shader) {}
};