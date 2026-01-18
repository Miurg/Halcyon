#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

constexpr int MAX_FRAMES_IN_FLIGHT = 3;
constexpr int MAX_OBJECTS = 3;
constexpr int MAX_PATH_LEN = 260;
constexpr int MAX_SIZE_OF_VERTEX_INDEX_BUFFER = 67108864; // 64 MB
constexpr int MAX_BINDLESS_TEXTURES = 1024;

struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};