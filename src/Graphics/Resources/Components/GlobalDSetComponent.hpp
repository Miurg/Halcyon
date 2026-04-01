#pragma once
#include "../Managers/ResourceHandles.hpp"

struct GlobalDSetComponent
{
	DSetHandle globalDSets;
	BufferHandle cameraBuffers;
	BufferHandle sunCameraBuffers;
	BufferHandle pointLightBuffers;
	BufferHandle pointLightCountBuffer;
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