#pragma once

#ifdef __cplusplus

#include <cstdint>
#include <glm/glm.hpp>

#include "HalcyonExport.hpp"

using uint = uint32_t;

using float2 = glm::vec2;
using float3 = glm::vec3;
using float4 = glm::vec4;
using int3 = glm::ivec3;

using float4x4 = glm::mat4;

#define GPU_ALIGN(n) alignas(n)
#define GPU_DEFAULT(...) = __VA_ARGS__

#else

#define GPU_ALIGN(n)
#define GPU_DEFAULT(...)
#define HALCYON_API

#endif
