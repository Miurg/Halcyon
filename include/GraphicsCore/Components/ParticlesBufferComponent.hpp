#pragma once
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"

struct ParticlesBufferComponent
{
	BufferHandle particlesBuffer;
	BufferHandle indirectBuffer;
	BufferHandle aliveIndicesBufferA;
	BufferHandle aliveIndicesBufferB;
	BufferHandle dispatchBuffer;
	BufferHandle emitersData;
};