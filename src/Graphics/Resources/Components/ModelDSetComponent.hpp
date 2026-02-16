#pragma once
#include "../Managers/ResourceHandles.hpp"

struct ModelDSetComponent
{
	DSetHandle modelBufferDSet;
	BufferHandle primitiveBuffer;
	BufferHandle transformBuffer;
	BufferHandle indirectDrawBuffer;
	BufferHandle visibleIndicesBuffer;
	BufferHandle totalIndicies;
};