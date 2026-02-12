#pragma once

struct LightComponent
{
	float sizeX = 4096;
	float sizeY = 4096;
	glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 3.0f);      // rgb: color, w: intensity of light
	glm::vec4 ambient = glm::vec4(1.0f, 1.0f, 1.0f, 0.1f);    // rgb: ambient color, w: intensity of ambient
	int textureShadowImage = -1;
	float shadowDistance = 30.0f;      // Distance at which shadows start to fade out
	float shadowCasterRange = 1000.0f; // Distance at which sun can cast shadows

	LightComponent() = default;

	LightComponent(int sizeX, int sizeY) : sizeX(sizeX), sizeY(sizeY) {};
	LightComponent(int sizeX, int sizeY, glm::vec4 color, glm::vec4 ambient)
		: sizeX(sizeX), sizeY(sizeY), color(color), ambient(ambient) {};
};