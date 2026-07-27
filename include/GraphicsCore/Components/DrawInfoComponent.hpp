#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Passes/DrawVariant.hpp"
#include <vector>
#include <cstdint>

struct HALCYON_API DrawInfoComponent
{
	uint32_t totalDrawCount = 0;
	uint32_t totalObjectCount = 0;
	std::vector<DrawSegment> segments;
};
