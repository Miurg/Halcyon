#pragma once

struct PointLightComponent
{
	float radius = 10.0f;
	glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
	float intensity = 1.0f;
	glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
	float innerConeAngle = glm::cos(glm::radians(15.0f));
	float outerConeAngle = glm::cos(glm::radians(30.0f));
	uint32_t type; // 0 = point, 1 = spot
};