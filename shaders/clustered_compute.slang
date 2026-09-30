#include "Shared/GpuStructs.h"
#include "Shared/Bindings.h"

// Threads per group, an occupancy knob only: every thread processes one whole cluster alone.
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

float getSliceDepth(
	uint boundary,
	float cameraNear,
	float depthRatio)
{
	float t = float(boundary) / float(Z_SLICES);
	return cameraNear * pow(depthRatio, t);
}

// TODO: checkup with depth_pyramid and maybe fix p32
float3 unprojectAtLinearDepth(
	float2 ndcXY,
	float linearDepth,
	float p22,
	float absP32,
	float4x4 invViewProj)
{
	float deviceDepth = saturate(absP32 / linearDepth - p22);

	float4 worldH = mul(
		invViewProj,
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

void buildClusterPlanes(
	uint3 clusterCoord,
	out float4 planes[CLUSTER_PLANE_COUNT])
{
	float p22 = camera[0].projMatrix[2][2];
	float absP32 = abs(camera[0].projMatrix[3][2]);
	float4x4 invViewProj = camera[0].invViewProj;

	float cameraNear = max(
		absP32 / max(abs(1.0f + p22), 1e-7f),
		1e-4f);
	float cameraFar = max(
		absP32 / max(abs(p22), 1e-7f),
		cameraNear + 1e-3f);
	float depthRatio = cameraFar / cameraNear;

	float2 screenSize = float2(
		push.widthScreen,
		push.heightScreen);

	float2 tileMinPx = float2(clusterCoord.xy * TILE_SIZE);
	float2 tileMaxPx = min(
		tileMinPx + float2(TILE_SIZE, TILE_SIZE),
		screenSize);

	float2 ndcMin = tileMinPx / screenSize * 2.0f - 1.0f;
	float2 ndcMax = tileMaxPx / screenSize * 2.0f - 1.0f;

	float sliceNear = getSliceDepth(
		clusterCoord.z,
		cameraNear,
		depthRatio);
	float sliceFar = getSliceDepth(
		clusterCoord.z + 1u,
		cameraNear,
		depthRatio);

	// 0/1/2/3 — near; 4/5/6/7 — far.
	float3 corners[8];

	corners[0] = unprojectAtLinearDepth(
		float2(ndcMin.x, ndcMin.y),
		sliceNear,
		p22,
		absP32,
		invViewProj);
	corners[1] = unprojectAtLinearDepth(
		float2(ndcMax.x, ndcMin.y),
		sliceNear,
		p22,
		absP32,
		invViewProj);
	corners[2] = unprojectAtLinearDepth(
		float2(ndcMin.x, ndcMax.y),
		sliceNear,
		p22,
		absP32,
		invViewProj);
	corners[3] = unprojectAtLinearDepth(
		float2(ndcMax.x, ndcMax.y),
		sliceNear,
		p22,
		absP32,
		invViewProj);

	corners[4] = unprojectAtLinearDepth(
		float2(ndcMin.x, ndcMin.y),
		sliceFar,
		p22,
		absP32,
		invViewProj);
	corners[5] = unprojectAtLinearDepth(
		float2(ndcMax.x, ndcMin.y),
		sliceFar,
		p22,
		absP32,
		invViewProj);
	corners[6] = unprojectAtLinearDepth(
		float2(ndcMin.x, ndcMax.y),
		sliceFar,
		p22,
		absP32,
		invViewProj);
	corners[7] = unprojectAtLinearDepth(
		float2(ndcMax.x, ndcMax.y),
		sliceFar,
		p22,
		absP32,
		invViewProj);

	float3 insidePoint = float3(0.0f);
	[unroll]
	for (uint i = 0u; i < 8u; ++i)
		insidePoint += corners[i];

	insidePoint *= 1.0f / 8.0f;

	planes[0] =
		makeInwardPlane(corners[0], corners[2], corners[6], insidePoint); // left

	planes[1] =
		makeInwardPlane(corners[1], corners[5], corners[7], insidePoint); // right

	planes[2] =
		makeInwardPlane(corners[0], corners[4], corners[5], insidePoint); // top

	planes[3] =
		makeInwardPlane(corners[2], corners[3], corners[7], insidePoint); // bottom

	planes[4] =
		makeInwardPlane(corners[0], corners[1], corners[3], insidePoint); // near

	planes[5] =
		makeInwardPlane(corners[4], corners[6], corners[7], insidePoint); // far
}

bool sphereIntersectsCluster(
	float3 center,
	float radius,
	float4 planes[CLUSTER_PLANE_COUNT])
{
	[unroll]
	for (uint i = 0u; i < CLUSTER_PLANE_COUNT; ++i)
	{
		float signedDistance =
			dot(planes[i], float4(center, 1.0f));

		if (signedDistance < -radius - CULL_EPSILON)
			return false;
	}

	return true;
}

bool aabbIntersectsCluster(
	float3 boxMin,
	float3 boxMax,
	float4 planes[CLUSTER_PLANE_COUNT])
{
	[unroll]
	for (uint i = 0u; i < CLUSTER_PLANE_COUNT; ++i)
	{
		float4 plane = planes[i];

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
void computeMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	if (push.widthScreen == 0u || push.heightScreen == 0u)
		return;

	uint2 tileCount = (
		uint2(push.widthScreen, push.heightScreen) +
		uint2(TILE_SIZE - 1u, TILE_SIZE - 1u)) /
		TILE_SIZE;

	uint tileXYCount = tileCount.x * tileCount.y;
	uint clusterCount = tileXYCount * Z_SLICES;

	uint clusterIndex = dispatchThreadID.x;
	if (clusterIndex >= clusterCount)
		return;

	// Decompose the flat index back into the
	// x + y * tileCount.x + z * tileCount.x * tileCount.y layout
	// that standard_forward uses to fetch the cluster.
	uint sliceZ = clusterIndex / tileXYCount;
	uint tileRemainder = clusterIndex - sliceZ * tileXYCount;
	uint tileY = tileRemainder / tileCount.x;
	uint tileX = tileRemainder - tileY * tileCount.x;

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
		ForwardCluster emptyCluster;
		emptyCluster.offset = INVALID_FORWARD_CLUSTER_OFFSET;
		emptyCluster.countLights = 0u;
		emptyCluster.countReflectProbes = 0u;
		forwardClusteredGrid[clusterIndex] = emptyCluster;

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

	float4 planes[CLUSTER_PLANE_COUNT];
	buildClusterPlanes(uint3(tileX, tileY, sliceZ), planes);

	// The thread owns the whole cluster, so compaction is a plain local append;
	// hits beyond MAX_FORWARD_CLUSTER_REFERENCES are only counted to keep the fallback logic intact.
	uint lightRefs[MAX_FORWARD_CLUSTER_REFERENCES];
	uint probeRefs[MAX_FORWARD_CLUSTER_REFERENCES];
	uint lightHitCount = 0u;
	uint probeHitCount = 0u;

	for (uint i = 0u; i < lightCount; ++i)
	{
		uint lightIndex = visiblePointLightIndices[1u + i];
		PointLightData light = pointLights[lightIndex];

		bool hit =
			light.radius > 0.0f &&
			sphereIntersectsCluster(light.position, light.radius, planes);

		if (hit)
		{
			if (lightHitCount < MAX_FORWARD_CLUSTER_REFERENCES)
				lightRefs[lightHitCount] = lightIndex;

			++lightHitCount;
		}
	}

	for (uint i = 0u; i < probeCount; ++i)
	{
		ReflectionProbeData probe = reflectionProbes[i];

		float3 boxMin = min(probe.boxMin, probe.boxMax);
		float3 boxMax = max(probe.boxMin, probe.boxMax);

		if (aabbIntersectsCluster(boxMin, boxMax, planes))
		{
			if (probeHitCount < MAX_FORWARD_CLUSTER_REFERENCES)
				probeRefs[probeHitCount] = i;

			++probeHitCount;
		}
	}

	uint totalCount = lightHitCount + probeHitCount;

	ForwardCluster cluster;
	cluster.offset = 0u;
	cluster.countLights = lightHitCount;
	cluster.countReflectProbes = probeHitCount;

	bool writeAccepted = true;

	if (totalCount > MAX_FORWARD_CLUSTER_REFERENCES)
	{
		writeAccepted = false;
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
			uint baseOffset =
				INFO_HEADER_WORDS + reservation;

			// Lights first, then probes, matching what standard_forward reads back.
			for (uint i = 0u; i < lightHitCount; ++i)
				forwardClusteredInfo[baseOffset + i] = lightRefs[i];

			for (uint i = 0u; i < probeHitCount; ++i)
				forwardClusteredInfo[baseOffset + lightHitCount + i] = probeRefs[i];

			cluster.offset = baseOffset;
		}
		else
		{
			writeAccepted = false;
			InterlockedOr(
				forwardClusteredInfo[1],
				FORWARD_CLUSTER_GLOBAL_OVERFLOW_BIT);
		}
	}

	if (!writeAccepted)
		cluster.offset = INVALID_FORWARD_CLUSTER_OFFSET;

	forwardClusteredGrid[clusterIndex] = cluster;
}
