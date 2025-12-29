#pragma once

#include <glm/glm.hpp>

struct TransformComponent
{
	glm::vec3 position = {0.0f, 0.0f, 0.0f};
	glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
	glm::vec3 scale = {1.0f, 1.0f, 1.0f};

	TransformComponent() = default;

	TransformComponent(glm::vec3 pos) : position(pos){}

	TransformComponent(glm::vec3 pos, glm::vec3 rot) : position(pos), rotation(rot) {}

	TransformComponent(glm::vec3 pos, glm::vec3 rot, glm::vec3 scl) : position(pos), rotation(rot), scale(scl) {}

	TransformComponent(float x, float y, float z)
		: position(glm::vec3(x, y, z)) {}

	TransformComponent(float px, float py, float pz, float rx, float ry, float rz)
	    : position(glm::vec3(px, py, pz)), rotation(glm::vec3(rx, ry, rz)) {}

	TransformComponent(float px, float py, float pz, float rx, float ry, float rz, float sx, float sy, float sz)
	    : position(glm::vec3(px, py, pz)), rotation(glm::vec3(rx, ry, rz)), scale(glm::vec3(sx, sy, sz)) {}

	TransformComponent(const TransformComponent& other)
	    : position(other.position), rotation(other.rotation), scale(other.scale) {}

	glm::mat4 getModelMatrix() const
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, position);
		model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, scale);
		return model;
	}
};