module GI;

// Reversed-Z: near/far swapped — perspectiveRH_ZO(90, 1, captureRange, 0.01) with Vulkan Y-flip, in analytic form
public static const float kBakeFar = 0.01f;

public static const float PROBE_NORMAL_BIAS = 0.25f;      // sample offset along N, in probe-spacing units
public static const float PROBE_BACKFACE_THRESHOLD = 0.5f; // _p0 above this = probe inside geometry

public static const float3 kFaceForward[6] = {
	float3(1, 0, 0), float3(-1, 0, 0), float3(0, 1, 0), float3(0, -1, 0), float3(0, 0, 1), float3(0, 0, -1),
};
public static const float3 kFaceUp[6] = {
	float3(0, 1, 0), float3(0, 1, 0), float3(0, 0, -1), float3(0, 0, 1), float3(0, 1, 0), float3(0, 1, 0),
};

public float4 bakeFaceClip(float3 worldPos, float3 probePos, uint faceIdx, float captureRange)
{
	float3 f = kFaceForward[faceIdx];
	float3 s = normalize(cross(f, kFaceUp[faceIdx]));
	float3 u = cross(s, f);
	float3 rel = worldPos - probePos;
	float3 view = float3(dot(s, rel), dot(u, rel), -dot(f, rel));

	// b = -(zFar*zNear)/(zFar-zNear), glm sign convention: positive with swapped reversed-Z near/far
	const float a = kBakeFar / (captureRange - kBakeFar);
	const float b = (kBakeFar * captureRange) / (captureRange - kBakeFar);
	return float4(view.x, -view.y, a * view.z + b, -view.z);
}
