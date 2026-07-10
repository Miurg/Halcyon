#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"
#include <glm/glm.hpp>

struct HALCYON_API ReflectionProbeComponent
{
	glm::vec3 origin = glm::vec3(0.0f);      // capture point (parallax reprojection center)
	glm::vec3 halfExtent = glm::vec3(5.0f);  // influence box half-size, world units
	float captureRange = 1000.0f;            // per-face draw distance during capture
	glm::vec3 giAmbientColor = glm::vec3(1.0f); // constant ambient baked into this capture, separate from the diffuse grid's
	float giAmbientIntensity = 0.0f;            // scales giAmbientColor; 0 = no constant ambient in the capture
	float giBounceMultiplier = 1.0f;            // scales GI light in this capture; 1 = physical
	bool needBake = true;

	// Assigned by the baker; -1 = not yet baked.
	int cubemapIndex = -1;
	TextureHandle prefilteredMap{-1};
};
