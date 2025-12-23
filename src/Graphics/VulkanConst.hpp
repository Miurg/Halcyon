#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

constexpr int MAX_FRAMES_IN_FLIGHT = 3;
constexpr int MAX_OBJECTS = 3;

struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};