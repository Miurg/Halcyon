#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"

struct HALCYON_API ModelDSetComponent
{
	DSetHandle modelBufferDSet;
	BufferHandle primitiveBuffer;
	BufferHandle transformBuffer;
	BufferHandle indirectDrawBuffer;
	BufferHandle visibleIndicesBuffer;
	BufferHandle drawCountBuffer;
	BufferHandle compactedDrawBuffer;
	BufferHandle totalIndicies;

	// GI bake: cull outputs split into one region per (probe, face); created on first bake
	DSetHandle bakeModelDSet;
	BufferHandle bakeIndirectDrawBuffer;
	BufferHandle bakeVisibleIndicesBuffer;
	BufferHandle bakeCompactedDrawBuffer;
	BufferHandle bakeDrawCountBuffer;
	bool bakeBuffersReady = false;
};