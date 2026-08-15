#include "Shared/GpuStructs.h"
#include "Shared/Bindings.h"

static const uint CLUSTER_THREAD_COUNT = 64u;
static const uint CLUSTER_PLANE_COUNT  = 6u;

// forwardClusteredInfo layout:
// [0] - number of occupied payload elements
// [1] - global payload overflow bit plus number of clusters using the local fallback
// [2...] - lights indices, then reflectionProbes indices
static const uint INFO_HEADER_WORDS = 2u;

static const float CULL_EPSILON = 1e-4f;

// === INPUT ===

[[vk::binding(BIND_GLOBAL_CAMERA, 0)]]
StructuredBuffer<CameraData> camera;

[[vk::binding(BIND_GLOBAL_POINT_LIGHTS, 0)]]
StructuredBuffer<PointLightData> pointLights;

[[vk::binding(BIND_GLOBAL_POINT_LIGHT_COUNT, 0)]]
StructuredBuffer<uint> pointLightCountBuffer;

[[vk::binding(BIND_GLOBAL_VISIBLE_POINT_LIGHTS, 0)]]
StructuredBuffer<uint> visiblePointLightIndices;

[[vk::binding(BIND_GLOBAL_REFLECTION_PROBES, 0)]]
StructuredBuffer<ReflectionProbeData> reflectionProbes;

[[vk::binding(BIND_GLOBAL_REFLECTION_PROBE_COUNT, 0)]]
StructuredBuffer<uint> reflectionProbeCountBuffer;


// === OUTPUT ===

[[vk::binding(BIND_GLOBAL_FORWARD_CLUSTERED_GRID, 0)]]
RWStructuredBuffer<ForwardCluster> forwardClusteredGrid;

[[vk::binding(BIND_GLOBAL_FORWARD_CLUSTERED_INFO, 0)]]
RWStructuredBuffer<uint> forwardClusteredInfo;


struct PushConstants
{
	uint widthScreen;
	uint heightScreen;
};

[[vk::push_constant]]
PushConstants push;

// The planes are directed towards the inside of the cluster.
groupshared float4 sharedClusterPlanes[CLUSTER_PLANE_COUNT];

groupshared uint sharedLightHits[MAX_POINT_LIGHTS];
groupshared uint sharedProbeHits[MAX_REFLECTION_PROBES];

groupshared uint sharedLightIndices[MAX_POINT_LIGHTS];
groupshared uint sharedProbeIndices[MAX_REFLECTION_PROBES];

groupshared uint sharedLightCount;
groupshared uint sharedProbeCount;
groupshared uint sharedBaseOffset;
groupshared uint sharedWriteAccepted;

float getCameraNear()
{
	float p22 = camera[0].projMatrix[2][2];
	float p32 = camera[0].projMatrix[3][2];

	return abs(p32) / max(abs(1.0f + p22), 1e-7f);
}

float getCameraFar()
{
	float p22 = camera[0].projMatrix[2][2];
	float p32 = camera[0].projMatrix[3][2];

	return abs(p32) / max(abs(p22), 1e-7f);
}

float getSliceDepth(uint boundary)
{
	float zNear = max(getCameraNear(), 1e-4f);
	float zFar  = max(getCameraFar(), zNear + 1e-3f);

	float t = float(boundary) / float(Z_SLICES);
	return zNear * pow(zFar / zNear, t);
}

// TODO: checkup with depth_pyramid and maybe fix p32
float3 unprojectAtLinearDepth(float2 ndcXY, float linearDepth)
{
	float p22 = camera[0].projMatrix[2][2];
	float p32 = camera[0].projMatrix[3][2];

    float deviceDepth = saturate(abs(p32) / linearDepth - p22);

	float4 worldH = mul(
		camera[0].invViewProj,
		float4(ndcXY, deviceDepth, 1.0f));

	return worldH.xyz / worldH.w;
}

float4 makeInwardPlane(
	float3 a,
	float3 b,
	float3 c,
	float3 insidePoint)
{
	float3 normal = cross(b - a, c - a);
	normal *= rsqrt(max(dot(normal, normal), 1e-20f));

	float distance = -dot(normal, a);
	float4 plane = float4(normal, distance);

	if (dot(plane, float4(insidePoint, 1.0f)) < 0.0f)
		plane = -plane;

	return plane;
}

void buildClusterPlanes(uint3 clusterCoord)
{
	float2 screenSize = float2(
		push.widthScreen,
		push.heightScreen);

	float2 tileMinPx = float2(clusterCoord.xy * TILE_SIZE);
	float2 tileMaxPx = min(
		tileMinPx + float2(TILE_SIZE, TILE_SIZE),
		screenSize);

	float2 ndcMin = tileMinPx / screenSize * 2.0f - 1.0f;
	float2 ndcMax = tileMaxPx / screenSize * 2.0f - 1.0f;

	float sliceNear = getSliceDepth(clusterCoord.z);
	float sliceFar  = getSliceDepth(clusterCoord.z + 1u);

	// 0/1/2/3 — near; 4/5/6/7 — far.
	float3 corners[8];

	corners[0] = unprojectAtLinearDepth(
		float2(ndcMin.x, ndcMin.y), sliceNear);
	corners[1] = unprojectAtLinearDepth(
		float2(ndcMax.x, ndcMin.y), sliceNear);
	corners[2] = unprojectAtLinearDepth(
		float2(ndcMin.x, ndcMax.y), sliceNear);
	corners[3] = unprojectAtLinearDepth(
		float2(ndcMax.x, ndcMax.y), sliceNear);

	corners[4] = unprojectAtLinearDepth(
		float2(ndcMin.x, ndcMin.y), sliceFar);
	corners[5] = unprojectAtLinearDepth(
		float2(ndcMax.x, ndcMin.y), sliceFar);
	corners[6] = unprojectAtLinearDepth(
		float2(ndcMin.x, ndcMax.y), sliceFar);
	corners[7] = unprojectAtLinearDepth(
		float2(ndcMax.x, ndcMax.y), sliceFar);

	float3 insidePoint = float3(0.0f);
	[unroll]
	for (uint i = 0u; i < 8u; ++i)
		insidePoint += corners[i];

	insidePoint *= 1.0f / 8.0f;

	sharedClusterPlanes[0] =
		makeInwardPlane(corners[0], corners[2], corners[6], insidePoint); // left

	sharedClusterPlanes[1] =
		makeInwardPlane(corners[1], corners[5], corners[7], insidePoint); // right

	sharedClusterPlanes[2] =
		makeInwardPlane(corners[0], corners[4], corners[5], insidePoint); // top

	sharedClusterPlanes[3] =
		makeInwardPlane(corners[2], corners[3], corners[7], insidePoint); // bottom

	sharedClusterPlanes[4] =
		makeInwardPlane(corners[0], corners[1], corners[3], insidePoint); // near

	sharedClusterPlanes[5] =
		makeInwardPlane(corners[4], corners[6], corners[7], insidePoint); // far
}

bool sphereIntersectsCluster(float3 center, float radius)
{
	[unroll]
	for (uint i = 0u; i < CLUSTER_PLANE_COUNT; ++i)
	{
		float signedDistance =
			dot(sharedClusterPlanes[i], float4(center, 1.0f));

		if (signedDistance < -radius - CULL_EPSILON)
			return false;
	}

	return true;
}

bool aabbIntersectsCluster(float3 boxMin, float3 boxMax)
{
	[unroll]
	for (uint i = 0u; i < CLUSTER_PLANE_COUNT; ++i)
	{
		float4 plane = sharedClusterPlanes[i];

		float3 supportPoint = float3(
			plane.x >= 0.0f ? boxMax.x : boxMin.x,
			plane.y >= 0.0f ? boxMax.y : boxMin.y,
			plane.z >= 0.0f ? boxMax.z : boxMin.z);

		if (dot(plane, float4(supportPoint, 1.0f)) < -CULL_EPSILON)
			return false;
	}

	return true;
}

[shader("compute")]
[numthreads(CLUSTER_THREAD_COUNT, 1, 1)]
void computeMain(
	uint3 groupID : SV_GroupID,
	uint groupIndex : SV_GroupIndex)
{
	if (push.widthScreen == 0u || push.heightScreen == 0u)
		return;

	uint2 tileCount = (
		uint2(push.widthScreen, push.heightScreen) +
		uint2(TILE_SIZE - 1u, TILE_SIZE - 1u)) /
		TILE_SIZE;

	if (groupID.x >= tileCount.x ||
	    groupID.y >= tileCount.y ||
	    groupID.z >= Z_SLICES)
	{
		return;
	}

	uint clusterIndex =
		groupID.x +
		groupID.y * tileCount.x +
		groupID.z * tileCount.x * tileCount.y;

	uint gridElementCount;
	uint gridElementStride;
	forwardClusteredGrid.GetDimensions(
		gridElementCount,
		gridElementStride);

	if (clusterIndex >= gridElementCount)
		return;

	uint infoElementCount;
	uint infoElementStride;
	forwardClusteredInfo.GetDimensions(
		infoElementCount,
		infoElementStride);

	if (infoElementCount < INFO_HEADER_WORDS)
	{
		if (groupIndex == 0u)
		{
			ForwardCluster emptyCluster;
			emptyCluster.offset = INVALID_FORWARD_CLUSTER_OFFSET;
			emptyCluster.countLights = 0u;
			emptyCluster.countReflectProbes = 0u;
			forwardClusteredGrid[clusterIndex] = emptyCluster;
		}

		return;
	}

	uint lightBufferCapacity;
	uint lightBufferStride;
	pointLights.GetDimensions(
		lightBufferCapacity,
		lightBufferStride);

	uint visibleLightBufferCapacity;
	uint visibleLightBufferStride;
	visiblePointLightIndices.GetDimensions(
		visibleLightBufferCapacity,
		visibleLightBufferStride);

	uint probeBufferCapacity;
	uint probeBufferStride;
	reflectionProbes.GetDimensions(
		probeBufferCapacity,
		probeBufferStride);

	uint lightCount = 0u;
	if (visibleLightBufferCapacity > 0u)
	{
		lightCount = min(
			visiblePointLightIndices[0],
			min(
				visibleLightBufferCapacity - 1u,
				min(lightBufferCapacity, MAX_POINT_LIGHTS)));
	}

	uint probeCount = min(
		reflectionProbeCountBuffer[0],
		min(probeBufferCapacity, MAX_REFLECTION_PROBES));

	if (groupIndex == 0u)
	{
		buildClusterPlanes(groupID);

		sharedLightCount = 0u;
		sharedProbeCount = 0u;
		sharedBaseOffset = 0u;
		sharedWriteAccepted = 1u;
	}

	GroupMemoryBarrierWithGroupSync();

	for (uint i = groupIndex;
	     i < lightCount;
	     i += CLUSTER_THREAD_COUNT)
	{
		uint lightIndex =
			visiblePointLightIndices[1u + i];
		PointLightData light =
			pointLights[lightIndex];

		sharedLightIndices[i] = lightIndex;
		sharedLightHits[i] =
			light.radius > 0.0f &&
			sphereIntersectsCluster(light.position, light.radius)
				? 1u
				: 0u;
	}

	for (uint i = groupIndex;
	     i < probeCount;
	     i += CLUSTER_THREAD_COUNT)
	{
		ReflectionProbeData probe = reflectionProbes[i];

		float3 boxMin = min(probe.boxMin, probe.boxMax);
		float3 boxMax = max(probe.boxMin, probe.boxMax);

		sharedProbeHits[i] =
			aabbIntersectsCluster(boxMin, boxMax)
				? 1u
				: 0u;
	}

	GroupMemoryBarrierWithGroupSync();

	if (groupIndex == 0u)
	{
		for (uint i = 0u; i < lightCount; ++i)
		{
			if (sharedLightHits[i] != 0u)
			{
				sharedLightIndices[sharedLightCount++] =
					sharedLightIndices[i];
			}
		}

		for (uint i = 0u; i < probeCount; ++i)
		{
			if (sharedProbeHits[i] != 0u)
				sharedProbeIndices[sharedProbeCount++] = i;
		}

		uint totalCount =
			sharedLightCount +
			sharedProbeCount;

		if (totalCount > MAX_FORWARD_CLUSTER_REFERENCES)
		{
			sharedWriteAccepted = 0u;
			InterlockedAdd(forwardClusteredInfo[1], 1u);
		}
		else if (totalCount != 0u)
		{
			uint reservation;
			InterlockedAdd(
				forwardClusteredInfo[0],
				totalCount,
				reservation);

			uint payloadCapacity =
				infoElementCount - INFO_HEADER_WORDS;

			bool fits = false;

			if (reservation <= payloadCapacity)
				fits = totalCount <= payloadCapacity - reservation;

			if (fits)
			{
				sharedBaseOffset =
					INFO_HEADER_WORDS + reservation;
			}
			else
			{
				sharedWriteAccepted = 0u;
				InterlockedOr(
					forwardClusteredInfo[1],
					FORWARD_CLUSTER_GLOBAL_OVERFLOW_BIT);
			}
		}
	}

	GroupMemoryBarrierWithGroupSync();

	if (sharedWriteAccepted != 0u)
	{
		for (uint i = groupIndex;
		     i < sharedLightCount;
		     i += CLUSTER_THREAD_COUNT)
		{
			forwardClusteredInfo[
				sharedBaseOffset + i] =
				sharedLightIndices[i];
		}

		for (uint i = groupIndex;
		     i < sharedProbeCount;
		     i += CLUSTER_THREAD_COUNT)
		{
			forwardClusteredInfo[
				sharedBaseOffset +
				sharedLightCount +
				i] =
				sharedProbeIndices[i];
		}
	}

	DeviceMemoryBarrierWithGroupSync();

	if (groupIndex == 0u)
	{
		ForwardCluster cluster;

		if (sharedWriteAccepted != 0u)
		{
			uint totalCount =
				sharedLightCount +
				sharedProbeCount;

			cluster.offset =
				totalCount != 0u
					? sharedBaseOffset
					: 0u;

			cluster.countLights =
				sharedLightCount;

			cluster.countReflectProbes =
				sharedProbeCount;
		}
		else
		{
			cluster.offset = INVALID_FORWARD_CLUSTER_OFFSET;
			cluster.countLights = sharedLightCount;
			cluster.countReflectProbes = sharedProbeCount;
		}

		forwardClusteredGrid[clusterIndex] = cluster;
	}
}
