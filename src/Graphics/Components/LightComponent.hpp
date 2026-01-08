#pragma once

struct LightComponent
{
	float sizeX = 0;
	float sizeY = 0;
	int textureShadowImage = -1;

	LightComponent() = default;

	LightComponent(int sizeX, int sizeY) : sizeX(sizeX), sizeY(sizeY) {};
};