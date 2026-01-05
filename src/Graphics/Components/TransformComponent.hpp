#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

struct TransformComponent
{
	glm::vec3 position = {0.0f, 0.0f, 0.0f};
	glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); 
	glm::vec3 scale = {1.0f, 1.0f, 1.0f};

	TransformComponent() = default;

	TransformComponent(glm::vec3 pos) : position(pos) {}

	//Eulers
	TransformComponent(glm::vec3 pos, glm::vec3 rotEuler) : position(pos), rotation(glm::quat(rotEuler)) {}

	TransformComponent(glm::vec3 pos, glm::quat rot) : position(pos), rotation(rot) {}

	TransformComponent(glm::vec3 pos, glm::vec3 rotEuler, glm::vec3 scl)
	    : position(pos), rotation(glm::quat(rotEuler)), scale(scl)
	{
	}

	TransformComponent(glm::vec3 pos, glm::quat rot, glm::vec3 scl) : position(pos), rotation(rot), scale(scl) {}

	TransformComponent(float x, float y, float z) : position(glm::vec3(x, y, z)) {}

	TransformComponent(float px, float py, float pz, float rx, float ry, float rz)
	    : position(glm::vec3(px, py, pz)), rotation(glm::quat(glm::vec3(rx, ry, rz)))
	{
	}

	TransformComponent(float px, float py, float pz, float rx, float ry, float rz, float sx, float sy, float sz)
	    : position(glm::vec3(px, py, pz)), rotation(glm::quat(glm::vec3(rx, ry, rz))), scale(glm::vec3(sx, sy, sz))
	{
	}

	TransformComponent(const TransformComponent& other)
	    : position(other.position), rotation(other.rotation), scale(other.scale)
	{
	}

	glm::mat4 getModelMatrix() const
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, position);
		model *= glm::mat4_cast(rotation);
		model = glm::scale(model, scale);
		return model;
	}

	glm::mat4 getViewMatrix() const
	{
		// R^T * T^-1
		glm::mat4 view = glm::mat4_cast(glm::conjugate(rotation));
		view = glm::translate(view, -position);
		return view;
	}

	void rotate(const glm::vec3& eulerAngles)
	{
		glm::quat q = glm::quat(eulerAngles);
		rotation = rotation * q;
		rotation = glm::normalize(rotation);
	}

	void rotate(float angle, const glm::vec3& axis)
	{
		glm::quat q = glm::angleAxis(angle, glm::normalize(axis));
		rotation = rotation * q;
		rotation = glm::normalize(rotation);
	}
};