#pragma once

struct SsaoSettingsComponent
{
	int kernelSize = 4;
	float radius = 6.0f;
	float bias = 0.3f;
	float power = 6.0f;
	int numDirections = 6;
	float maxScreenRadius = 0.5f;

	SsaoSettingsComponent() = default;
};
