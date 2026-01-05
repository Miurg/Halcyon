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
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;

	TransformComponent() = default;

	TransformComponent(glm::vec3 pos) : position(pos) 
	{
		updateDirectionVectors();
	}

	//Eulers
	TransformComponent(glm::vec3 pos, glm::vec3 rotEuler) : position(pos), rotation(glm::quat(rotEuler)) 
	{
		updateDirectionVectors();
	}

	TransformComponent(glm::vec3 pos, glm::quat rot) : position(pos), rotation(rot) 
	{
		updateDirectionVectors();
	}

	TransformComponent(glm::vec3 pos, glm::vec3 rotEuler, glm::vec3 scl)
	    : position(pos), rotation(glm::quat(rotEuler)), scale(scl)
	{
		updateDirectionVectors();
	}

	TransformComponent(glm::vec3 pos, glm::quat rot, glm::vec3 scl) : position(pos), rotation(rot), scale(scl) 
	{
		updateDirectionVectors();
	}

	TransformComponent(float x, float y, float z) : position(glm::vec3(x, y, z)) 
	{
		updateDirectionVectors();
	}

	TransformComponent(float px, float py, float pz, float rx, float ry, float rz)
	    : position(glm::vec3(px, py, pz)), rotation(glm::quat(glm::vec3(rx, ry, rz)))
	{
		updateDirectionVectors();
	}

	TransformComponent(float px, float py, float pz, float rx, float ry, float rz, float sx, float sy, float sz)
	    : position(glm::vec3(px, py, pz)), rotation(glm::quat(glm::vec3(rx, ry, rz))), scale(glm::vec3(sx, sy, sz))
	{
		updateDirectionVectors();
	}

	TransformComponent(const TransformComponent& other)
	    : position(other.position), rotation(other.rotation), scale(other.scale)
	{
		updateDirectionVectors();
	}

	void updateDirectionVectors()
	{
		front = glm::rotate(rotation, glm::vec3(0.0f, 0.0f, -1.0f));
		up = glm::rotate(rotation, glm::vec3(0.0f, 1.0f, 0.0f));
		right = glm::rotate(rotation, glm::vec3(1.0f, 0.0f, 0.0f));
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

	// Local euler
	void rotateLocal(const glm::vec3& eulerAngles)
	{
		glm::quat q = glm::quat(eulerAngles);
		rotation = rotation * q;
		rotation = glm::normalize(rotation);
		updateDirectionVectors();
	}

	// Local
	void rotateLocal(float angle, const glm::vec3& axis)
	{
		glm::quat q = glm::angleAxis(angle, glm::normalize(axis));
		rotation = rotation * q;
		rotation = glm::normalize(rotation);
		updateDirectionVectors();
	}

	// Global euler
	void rotateGlobal(const glm::vec3& eulerAngles)
	{
		glm::quat q = glm::quat(eulerAngles);
		rotation = q * rotation;
		rotation = glm::normalize(rotation);
		updateDirectionVectors();
	}

	// Global
	void rotateGlobal(float angle, const glm::vec3& axis)
	{
		glm::quat q = glm::angleAxis(angle, glm::normalize(axis));
		rotation = q * rotation;
		rotation = glm::normalize(rotation);
		updateDirectionVectors();
	}
};