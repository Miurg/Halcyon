#pragma once

#include <vector>
#include "../FrameData.hpp"

struct FrameDataComponent
{
	std::vector<FrameData>* frameDataArray;

	FrameDataComponent(std::vector<FrameData>* frameData) : frameDataArray(frameData) {}
};