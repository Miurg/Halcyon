#pragma once

#include "GpuTypes.h"

struct HALCYON_API CameraData
{
	float4x4 cameraSpaceMatrix; // view * projection
	float4x4 viewMatrix;
	float4x4 projMatrix;
	float4x4 invViewProj;
	float4 cameraPositionAndPadding;
	float4 frustumPlanes[6];
};

struct HALCYON_API IndirectDrawIndexedCommand
{
	uint indexCount;
	uint instanceCount;
	uint firstIndex;
	int vertexOffset;
	uint firstInstance;
};

struct HALCYON_API IndirectDrawCommand
{
	uint vertexCount;
	uint instanceCount;
	uint firstVertex;
	uint firstInstance;
};

struct HALCYON_API IndirectDispatchCommand
{
	uint x;
	uint y;
	uint z;
	uint spawnCount;
};

struct HALCYON_API DirectionalLightData
{
	float4x4 lightSpaceMatrix;
	float4 direction;
	float4 color;   // rgb: color, a: intensity
	float4 ambient; // rgb: color, a: intensity
	float4 shadowMapSize; // x: width, y: height, z: 1/width, w: 1/height
	float4 frustumPlanes[6];
	float shadowCasterRange;
	float _pad1;
	float _pad2;
	float _pad3;
	float4 cameraFrustumLightSpaceBounds;
};

struct HALCYON_API PointLightData
{
	float3 position;
	float radius;
	float3 color;
	float intensity;
	float3 direction;     // spot: normalized direction; point: ignored
	float innerConeAngle; // point: -1
	float outerConeAngle; // point: -1
	uint type;            // 0 = point, 1 = spot
	uint _pad1;
	uint _pad2;
};

struct HALCYON_API GPU_ALIGN(16) ModelData
{
	float3 AABBMin;
	float padding0;
	float3 AABBMax;
	float padding1;
	uint transformIndex;
	uint materialIndex;
	uint drawCommandIndex;
};

struct HALCYON_API TransformData
{
	float4x4 model;
};

struct HALCYON_API GPU_ALIGN(16) SHGridInfo
{
	float3 origin;
	float spacing;
	int3 count;
	uint probeCount;
	float3 giAmbient;
	float captureRange;
	float giBounceMultiplier;
};

// Slot 0 = skybox fallback (influenceRadius = FLT_MAX, position ignored).
struct HALCYON_API SHProbeEntry
{
	float3 position;
	float influenceRadius;
	float3 sh0;
	float _p0; // backface fraction — probe validity
	float3 sh1;
	float _p1;
	float3 sh2;
	float _p2;
	float3 sh3;
	float _p3;
};

struct HALCYON_API ReflectionProbeData
{
	float3 boxMin;
	uint cubemapIndex;
	float3 boxMax;
	float _pad0;
	float3 captureOrigin;
	float _pad1;
};

struct HALCYON_API MaterialData
{
	uint textureIndex GPU_DEFAULT(~0u);
	uint normalMapIndex GPU_DEFAULT(~0u);
	uint metallicRoughnessIndex GPU_DEFAULT(~0u);
	uint emissiveIndex GPU_DEFAULT(~0u);
	float alphaCutoff GPU_DEFAULT(0.5f);
	uint alphaMode GPU_DEFAULT(0); // 0 = OPAQUE, 1 = MASK, 2 = BLEND
	float emissiveStrength GPU_DEFAULT(1.0f);
	uint doubleSided GPU_DEFAULT(0);
	float4 baseColorFactor GPU_DEFAULT({1.0f, 1.0f, 1.0f, 1.0f});
	float3 emissiveFactor GPU_DEFAULT({0.0f, 0.0f, 0.0f});
	float emissivePadding GPU_DEFAULT(0.0f);
	float roughnessFactor GPU_DEFAULT(1.0f);
	float metallicFactor GPU_DEFAULT(1.0f);
	float padding1 GPU_DEFAULT(0.0f);
	float padding2 GPU_DEFAULT(0.0f);
};

#ifdef __cplusplus
static_assert(sizeof(CameraData) == 368);
static_assert(sizeof(IndirectDrawIndexedCommand) == 20);
static_assert(sizeof(IndirectDrawCommand) == 16);
static_assert(sizeof(IndirectDispatchCommand) == 16);
static_assert(sizeof(DirectionalLightData) == 256);
static_assert(sizeof(PointLightData) == 64);
static_assert(sizeof(ModelData) == 48);
static_assert(sizeof(TransformData) == 64);
static_assert(sizeof(SHGridInfo) == 64);
static_assert(sizeof(SHProbeEntry) == 80);
static_assert(sizeof(ReflectionProbeData) == 48);
static_assert(sizeof(MaterialData) == 80);
#endif
