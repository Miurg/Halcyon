#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Passes/DrawVariant.hpp"
#include <glm/fwd.hpp>
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"

// Stored in MeshInfo
struct HALCYON_API PrimitivesInfo
{
	uint32_t vertexOffset = -1;
	uint32_t indexOffset = -1;
	uint32_t indexCount = -1;
	MaterialHandle materialIndex;
	DrawVariantIndex drawVariant = DrawVariantIndex::Opaque;
	glm::vec3 AABBMin;
	glm::vec3 AABBMax;
};