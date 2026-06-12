#pragma once
#include "../Managers/ResourceHandles.hpp"

struct GlobalDSetComponent
{
	DSetHandle globalDSets;
	BufferHandle cameraBuffers;
	BufferHandle sunCameraBuffers;
	BufferHandle pointLightBuffers;
	BufferHandle pointLightCountBuffer;
	BufferHandle shProbeBuffer;      // SHProbeEntry[MAX_SH_PROBES] — slot 0 = skybox fallback
	BufferHandle shProbeCountBuffer; // single uint32_t
};