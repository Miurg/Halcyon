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
	DSetHandle ssaoBlurDSets;
	DSetHandle ssaoApplyDSets;
	DSetHandle toneMappingDSets;
};