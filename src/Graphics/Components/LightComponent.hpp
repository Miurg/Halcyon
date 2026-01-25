#pragma once

struct LightComponent
{
	float sizeX = 0;
	float sizeY = 0;
	glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 3.0f);      // rgb: color, w: intensity of light
	glm::vec4 ambient = glm::vec4(1.0f, 1.0f, 1.0f, 0.1f);    // rgb: ambient color, w: intensity of ambient
	int textureShadowImage = -1;

	LightComponent() = default;

	LightComponent(int sizeX, int sizeY) : sizeX(sizeX), sizeY(sizeY) {};
};