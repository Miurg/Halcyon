#pragma once

struct CameraStructure
{
	alignas(16) glm::mat4 cameraSpaceMatrix;
	alignas(16) glm::vec3 cameraPosition;
};

struct SunStructure
{
	alignas(16) glm::mat4 lightSpaceMatrix;
	glm::vec4 direction; // xyz: direction, w: padding
	glm::vec4 color;     // rgb: color, w: intensity of light
	glm::vec4 ambient;   // rgb: ambient color, w: intensity of ambient
};

// GPU-side per-primitive data (64 bytes, 16-byte aligned). Matches shader SSBO layout.
struct PrimitiveSctructure // (16 + 16 + 16 + 4 + 4 + 4 + 4 = 64 bytes)
{
	alignas(16) glm::vec3 AABBMin;   // xyz: min
	float padding0;                  // w: padding
	alignas(16) glm::vec3 AABBMax;   // xyz: max
	float padding1;                  // w: padding
	uint32_t transformIndex;         // index to the transform of the primitive
	uint32_t materialIndex;           // index to the material of the primitive
	uint32_t drawCommandIndex;       // index to the indirect draw command of the primitive
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
	uint32_t textureIndex = -1;
	uint32_t normalMapIndex = -1;
	uint32_t metallicRoughnessIndex = -1;
};