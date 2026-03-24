#pragma once

struct SsaoSettingsComponent
{
	int kernelSize = 4;
	float radius = 2.0f;
	float bias = 0.3f;
	float power = 2.0f;
	int numDirections = 6;
	float maxScreenRadius = 0.5f;
	float fadeStart = 300.0f;
	float fadeEnd = 500.0f;

	SsaoSettingsComponent() = default;
};
