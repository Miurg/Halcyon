#pragma once

#include "HalcyonExport.hpp"
#include <glm/fwd.hpp>

struct HALCYON_API CameraStructure
{
	alignas(16) glm::mat4 cameraSpaceMatrix;
	alignas(16) glm::mat4 viewMatrix;
	alignas(16) glm::mat4 projMatrix;
	alignas(16) glm::mat4 invViewProj;
	alignas(16) glm::vec4 cameraPositionAndPadding;
	alignas(16) glm::vec4 frustumPlanes[6];
};

struct HALCYON_API DirectLightStructure
{
	alignas(16) glm::mat4 lightSpaceMatrix;
	alignas(16) glm::vec4 direction; // xyz: direction, w: padding
	alignas(16) glm::vec4 color;     // rgb: color, w: intensity of light
	alignas(16) glm::vec4 ambient;   // rgb: ambient color, w: intensity of ambient
	alignas(16) glm::vec4 shadowMapSize; // x: width, y: height, z: 1.0/width, w: 1.0/height
	alignas(16) glm::vec4 frustumPlanes[6];
	float shadowCasterRange;
	float _pad1;
	float _pad2;
	float _pad3;
	alignas(16) glm::vec4 cameraFrustumLightSpaceBounds;
};

// GPU-side per-primitive data (64 bytes, 16-byte aligned). Matches shader SSBO layout.
struct HALCYON_API PrimitiveSctructure // (16 + 16 + 16 + 4 + 4 + 4 + 4 = 64 bytes)
{
	alignas(16) glm::vec3 AABBMin; // xyz: min
	float padding0;                // w: padding
	alignas(16) glm::vec3 AABBMax; // xyz: max
	float padding1;                // w: padding
	uint32_t transformIndex;       // index to the transform of the primitive
	uint32_t materialIndex;        // index to the material of the primitive
	uint32_t drawCommandIndex;     // index to the indirect draw command of the primitive
};

struct HALCYON_API TransformStructure
{
	alignas(16) glm::mat4 model;
};

// Mirrors VkDrawIndexedIndirectCommand for GPU-driven indirect draws.
struct HALCYON_API IndirectDrawStructure
{
	uint32_t indexCount;
	uint32_t instanceCount;
	uint32_t firstIndex;
	int vertexOffset;
	uint32_t firstInstance;
};

struct HALCYON_API MaterialStructure
{
	uint32_t textureIndex = ~0u;
	uint32_t normalMapIndex = ~0u;
	uint32_t metallicRoughnessIndex = ~0u;
	uint32_t emissiveIndex = ~0u;
	float alphaCutoff = 0.5f;
	uint32_t alphaMode = 0; // 0 = OPAQUE, 1 = MASK, 2 = BLEND
	float emissiveStrength = 1.0f;
	uint32_t doubleSided = 0; // 0 = single-sided, 1 = double-sided
	alignas(16) glm::vec4 baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f};
	alignas(16) glm::vec3 emissiveFactor = {0.0f, 0.0f, 0.0f};
	float emissivePadding = 0.0f;
	float roughnessFactor = 1.0f;
	float metallicFactor = 1.0f;
	float padding1 = 0.0f;
	float padding2 = 0.0f;
};

// Slot 0 = skybox fallback (influenceRadius = FLT_MAX, position ignored).
struct HALCYON_API SHProbeEntry
{
    alignas(16) glm::vec3 position;
    float influenceRadius; // FLT_MAX = global fallback (skybox slot)

    alignas(16) glm::vec3 sh0; float _p0; // _p0 = backface fraction (probe validity)
    alignas(16) glm::vec3 sh1; float _p1;
    alignas(16) glm::vec3 sh2; float _p2;
    alignas(16) glm::vec3 sh3; float _p3;
    // Total: 5 × 16 = 80 bytes
};

// std430 layout — must match SHGridInfo in the GI shaders (64 bytes). giAmbient is a float3, which
// std430 aligns to the 16-byte boundary after captureRange, hence alignas(16).
struct HALCYON_API SHGridInfo
{
    glm::vec3 origin;
    float spacing;
    glm::ivec3 count;
    uint32_t probeCount; // grid probes + skybox slot 0; 1 = nothing baked
    float captureRange;
    alignas(16) glm::vec3 giAmbient; // GI-only constant ambient (color × intensity), baked in instead of the sun's ambient
    float giBounceMultiplier;        // scales GI sampled by the bake itself (multi-bounce feedback)
};

// Must match ReflectionProbeData in standard_forward.slang (std430, 48 bytes).
struct HALCYON_API ReflectionProbeData
{
    alignas(16) glm::vec3 boxMin;
    uint32_t cubemapIndex;
    alignas(16) glm::vec3 boxMax;
    float _pad0;
    alignas(16) glm::vec3 captureOrigin; // point the cubemap was captured from (parallax reprojection center)
    float _pad1;
};

struct HALCYON_API PointLightStructure
{
	alignas(16) glm::vec3 position;
	float radius;
	alignas(16) glm::vec3 color;
	float intensity;
	alignas(16) glm::vec3 direction; 
	float innerConeAngle;
	float outerConeAngle;
	uint32_t type;        // 0 = point, 1 = spot
	uint32_t _pad1;
	uint32_t _pad2;
};