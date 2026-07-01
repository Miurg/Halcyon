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
};