#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"

struct HALCYON_API GlobalDSetComponent
{
	DSetHandle globalDSets;
	BufferHandle cameraBuffers;
	BufferHandle sunCameraBuffers;
	BufferHandle pointLightBuffers;
	BufferHandle pointLightCountBuffer;
	BufferHandle shProbeBuffer;      // SHProbeEntry[MAX_SH_PROBES] — slot 0 = skybox fallback
	BufferHandle shGridInfoBuffer;   // SHGridInfo — probeCount includes skybox slot 0
};