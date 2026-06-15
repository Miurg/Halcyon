#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"

struct HALCYON_API ParticlesBufferComponent
{
	BufferHandle particlesBuffer;
	BufferHandle indirectBuffer;
	BufferHandle aliveIndicesBufferA;
	BufferHandle aliveIndicesBufferB;
	BufferHandle dispatchBuffer;
	BufferHandle emitersData;
};