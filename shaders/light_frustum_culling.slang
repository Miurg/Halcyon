#include "Shared/GpuStructs.h"
#include "Shared/Bindings.h"

static const uint LIGHT_CULL_THREAD_COUNT = 64u;
static const float CULL_EPSILON = 1e-4f;

[[vk::binding(BIND_GLOBAL_CAMERA, 0)]]
StructuredBuffer<CameraData> camera;

[[vk::binding(BIND_GLOBAL_POINT_LIGHTS, 0)]]
StructuredBuffer<PointLightData> pointLights;

[[vk::binding(BIND_GLOBAL_POINT_LIGHT_COUNT, 0)]]
StructuredBuffer<uint> pointLightCountBuffer;

[[vk::binding(BIND_GLOBAL_VISIBLE_POINT_LIGHTS, 0)]]
RWStructuredBuffer<uint> visiblePointLightIndices;

bool sphereIntersectsCameraFrustum(float3 center, float radius)
{
	[unroll]
	for (uint i = 0u; i < 6u; ++i)
	{
		float signedDistance =
			dot(camera[0].frustumPlanes[i], float4(center, 1.0f));

		if (signedDistance < -radius - CULL_EPSILON)
			return false;
	}

	return true;
}

[shader("compute")]
[numthreads(LIGHT_CULL_THREAD_COUNT, 1, 1)]
void computeMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	uint lightBufferCapacity;
	uint lightBufferStride;
	pointLights.GetDimensions(
		lightBufferCapacity,
		lightBufferStride);

	uint visibleBufferCapacity;
	uint visibleBufferStride;
	visiblePointLightIndices.GetDimensions(
		visibleBufferCapacity,
		visibleBufferStride);

	if (visibleBufferCapacity <= 1u)
		return;

	uint lightCount = min(
		pointLightCountBuffer[0],
		min(lightBufferCapacity, MAX_POINT_LIGHTS));

	uint lightIndex = dispatchThreadID.x;
	if (lightIndex >= lightCount)
		return;

	PointLightData light = pointLights[lightIndex];
	if (light.radius <= 0.0f ||
		!sphereIntersectsCameraFrustum(light.position, light.radius))
	{
		return;
	}

	uint compactedIndex;
	InterlockedAdd(
		visiblePointLightIndices[0],
		1u,
		compactedIndex);

	if (compactedIndex < visibleBufferCapacity - 1u)
	{
		visiblePointLightIndices[
			1u + compactedIndex] = lightIndex;
	}
}
