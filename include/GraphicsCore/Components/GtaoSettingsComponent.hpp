#pragma once

struct GtaoSettingsComponent
{
	int kernelSize = 4;
	float radius = 2.0f;
	float bias = 0.3f;
	float power = 1.0f;
	int numDirections = 6;
	float maxScreenRadius = 0.5f;
	float fadeStart = 300.0f;
	float fadeEnd = 500.0f;
	float mipBias = -2.0f;
	float blurDepthTolerance = 0.05f;

	// anti-halo
	float pyramidEdgeRange = 0.1f;

	float multiBounceAlbedo = 0.5f;
	float thicknessScale = 1.5f;

	GtaoSettingsComponent() = default;
};
