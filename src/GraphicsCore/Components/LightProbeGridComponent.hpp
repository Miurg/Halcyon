#pragma once
#include <glm/glm.hpp>

// Defines a uniform 3-D grid of SH light probes.
// Slot 0 of shProbes[] is always the skybox fallback (infinite radius).
// Grid probes occupy slots 1 .. count.x*count.y*count.z.
struct LightProbeGridComponent
{
	glm::vec3 origin;            // world-space corner of the grid
	glm::ivec3 count;            // probes per axis
	float spacing;               // meters between adjacent probes
	bool needBake = true;        // when true - grid will rebake
	bool debugVisualize = false; // draw debug spheres at probe positions
	float debugScale = 0.3f;     // radius of debug spheres in meters
};
