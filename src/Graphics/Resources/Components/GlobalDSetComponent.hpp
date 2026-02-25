#pragma once
#include "../Managers/ResourceHandles.hpp"

struct GlobalDSetComponent
{
	DSetHandle globalDSets;
	BufferHandle cameraBuffers;
	BufferHandle sunCameraBuffers;
	DSetHandle fxaaDSets;
	DSetHandle ssaoDSets;
	DSetHandle ssaoBlurDSets;
};