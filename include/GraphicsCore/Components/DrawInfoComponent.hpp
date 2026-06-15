#pragma once

#include "HalcyonExport.hpp"
#include <cstdint>

struct HALCYON_API DrawInfoComponent
{
	uint32_t totalDrawCount = 0;
	uint32_t totalObjectCount = 0;

	uint32_t opaqueSingleCount = 0;
	uint32_t opaqueDoubleCount = 0;
	uint32_t maskSingleCount = 0;
	uint32_t maskDoubleCount = 0;
	uint32_t blendSingleCount = 0;
	uint32_t blendDoubleCount = 0;
};
