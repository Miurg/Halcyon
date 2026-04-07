#pragma once

struct CameraStructure
{
	alignas(16) glm::mat4 cameraSpaceMatrix;
	alignas(16) glm::mat4 viewMatrix;
	alignas(16) glm::mat4 projMatrix;
	alignas(16) glm::mat4 invViewProj;
	alignas(16) glm::vec4 cameraPositionAndPadding;
	alignas(16) glm::vec4 frustumPlanes[6];
};

struct DirectLightStructure
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
struct PrimitiveSctructure // (16 + 16 + 16 + 4 + 4 + 4 + 4 = 64 bytes)
{
	alignas(16) glm::vec3 AABBMin; // xyz: min
	float padding0;                // w: padding
	alignas(16) glm::vec3 AABBMax; // xyz: max
	float padding1;                // w: padding
	uint32_t transformIndex;       // index to the transform of the primitive
	uint32_t materialIndex;        // index to the material of the primitive
	uint32_t drawCommandIndex;     // index to the indirect draw command of the primitive
};

struct TransformStructure
{
	alignas(16) glm::mat4 model;
};

// Mirrors VkDrawIndexedIndirectCommand for GPU-driven indirect draws.
struct IndirectDrawStructure
{
	uint32_t indexCount;
	uint32_t instanceCount;
	uint32_t firstIndex;
	int vertexOffset;
	uint32_t firstInstance;
};

struct MaterialStructure
{
	uint32_t textureIndex = ~0u;
	uint32_t normalMapIndex = ~0u;
	uint32_t metallicRoughnessIndex = ~0u;
	uint32_t emissiveIndex = ~0u;
	float alphaCutoff = 0.5f;
	uint32_t alphaMode = 0; // 0 = OPAQUE, 1 = MASK, 2 = BLEND
	float emissiveStrength = 1.0f;
	uint32_t doubleSided = 0; // 0 = single-sided, 1 = double-sided
	alignas(16) glm::vec3 emissiveFactor = {0.0f, 0.0f, 0.0f};
	float emissivePadding = 0.0f;
	float roughnessFactor = 1.0f;
	float metallicFactor = 1.0f;
	float padding1 = 0.0f;
	float padding2 = 0.0f;
};

struct PointLightStructure
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