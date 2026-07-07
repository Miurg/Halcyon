#pragma once

#include "HalcyonExport.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

constexpr int MAX_FRAMES_IN_FLIGHT = 3;
constexpr int MAX_OBJECTS = 3;
constexpr int MAX_PATH_LEN = 260;
constexpr int MAX_BINDLESS_TEXTURES = 2048;
constexpr uint32_t MAX_SH_PROBES = 40960; // 127 scene probes + slot 0 (skybox fallback)
constexpr uint32_t MAX_REFLECTION_PROBES = 32;

struct HALCYON_API UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};