#pragma once

struct SsaoSettingsComponent
{
	int kernelSize = 8;
	float radius = 4.0f;
	float bias = 0.3f;
	float power = 8.0f;
	int numDirections = 12;
	float maxScreenRadius = 0.5f;

	SsaoSettingsComponent() = default;
};
