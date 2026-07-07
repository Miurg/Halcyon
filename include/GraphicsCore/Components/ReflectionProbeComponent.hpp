#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"
#include <glm/glm.hpp>

struct HALCYON_API ReflectionProbeComponent
{
	glm::vec3 origin = glm::vec3(0.0f);      // capture point (parallax reprojection center)
	glm::vec3 halfExtent = glm::vec3(5.0f);  // influence box half-size, world units
	float captureRange = 1000.0f;            // per-face draw distance during capture
	bool needBake = true;

	// Assigned by the baker; -1 = not yet baked.
	int cubemapIndex = -1;
	TextureHandle prefilteredMap{-1};
};
