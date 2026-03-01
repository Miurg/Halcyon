#pragma once

struct SsaoSettingsComponent
{
	int kernelSize = 6;
	float radius = 7.0f;
	float bias = 0.3f;
	float power = 7.0f;
	int numDirections = 6;
	float maxScreenRadius = 0.5f;

	SsaoSettingsComponent() = default;
};
