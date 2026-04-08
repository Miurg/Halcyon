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
	DSetHandle fxaaDSets;
	DSetHandle ssaoDSets;
	DSetHandle ssaoBlurHDSets;
	DSetHandle ssaoBlurVDSets;
	DSetHandle ssaoApplyDSets;
	DSetHandle toneMappingDSets;
	DSetHandle bloomDownsampleDSets[5];
	DSetHandle bloomUpsampleDSets[5];
	DSetHandle vignetteDSets;
};