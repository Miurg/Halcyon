#pragma once
#include "../../Core/Components/ComponentManager.h"

class MeshAsset;
class MaterialAsset;

struct RenderableComponent : Component
{
	MeshAsset* Mesh;
	MaterialAsset* Material;
	uint32_t batchIndex = UINT32_MAX;
	RenderableComponent(MeshAsset* m, MaterialAsset* mat) : Mesh(m), Material(mat) {}
};