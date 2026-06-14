#pragma once

struct CurrentFrameComponent
{
	uint32_t currentFrame = 0; // Current frame in frame in flight
	bool frameValid = false;
	uint32_t frameNumber = 0; // This is total number of frame that already pass
};