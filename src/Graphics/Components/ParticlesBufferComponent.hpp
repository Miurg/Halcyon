#pragma once
#include "../Resources/Managers/ResourceHandles.hpp"

struct ParticlesBufferComponent
{
	BufferHandle particlesBuffer;
	BufferHandle indirectBuffer;
	BufferHandle aliveIndicesBuffer;
};