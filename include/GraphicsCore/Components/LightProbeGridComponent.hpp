#pragma once

#include "HalcyonExport.hpp"
#include <glm/glm.hpp>

// Defines a uniform 3-D grid of SH light probes.
// Slot 0 of shProbes[] is always the skybox fallback (infinite radius).
// Grid probes occupy slots 1 .. count.x*count.y*count.z.
struct HALCYON_API LightProbeGridComponent
{
	glm::vec3 origin;            // world-space corner of the grid
	glm::ivec3 count;            // probes per axis
	float spacing;               // meters between adjacent probes
	float captureRange = 10.0f; // per-face draw distance; farther geometry is culled, skybox fills in
	glm::vec3 giAmbientColor = glm::vec3(1.0f); // GI-only ambient color grade, baked in instead of the sun's ambient
	float giAmbientIntensity = 0.0f;            // scales giAmbientColor; 0 = no constant GI ambient
	bool needBake = true;        // when true - grid will rebake
	bool debugVisualize = false; // draw debug spheres at probe positions
	float debugScale = 0.3f;     // radius of debug spheres in meters
};
