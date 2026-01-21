#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
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
	mutable glm::mat4 model = glm::mat4(1.0f); 
	mutable glm::mat4 view = glm::mat4(1.0f); 
	mutable bool isModelDirty = true;
	mutable bool isViewDirty = true;

	TransformComponent() = default;

	TransformComponent(glm::vec3 pos) : position(pos) 
	{
		updateDirectionVectors();
	}

	//Eulers
	TransformComponent(glm::vec3 pos, glm::vec3 rotEuler) : position(pos), rotation(glm::quat(glm::radians(rotEuler))) 
	{
		updateDirectionVectors();
	}

	TransformComponent(glm::vec3 pos, glm::quat rot) : position(pos), rotation(rot) 
	{
		updateDirectionVectors();
	}

	TransformComponent(glm::vec3 pos, glm::vec3 rotEuler, glm::vec3 scl)
	    : position(pos), rotation(glm::quat(glm::radians(rotEuler))), scale(scl)
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
	    : position(glm::vec3(px, py, pz)), rotation(glm::quat(glm::radians(glm::vec3(rx, ry, rz))))
	{
		updateDirectionVectors();
	}

	TransformComponent(float px, float py, float pz, float rx, float ry, float rz, float sx, float sy, float sz)
	    : position(glm::vec3(px, py, pz)), rotation(glm::quat(glm::radians(glm::vec3(rx, ry, rz)))),
	      scale(glm::vec3(sx, sy, sz))
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
		isModelDirty = true;
		isViewDirty = true;
	}

	const glm::mat4& getModelMatrix() const
	{
		if (isModelDirty)
		{
			// R * S + T
			const glm::mat3 rotationMatrix = glm::mat3_cast(rotation);
			// X axis
			model[0] = glm::vec4(rotationMatrix[0] * scale.x, 0.0f);
			// Y axis
			model[1] = glm::vec4(rotationMatrix[1] * scale.y, 0.0f);
			// Z axis
			model[2] = glm::vec4(rotationMatrix[2] * scale.z, 0.0f);
			// Position
			model[3] = glm::vec4(position, 1.0f);

			isModelDirty = false;
		}
		return model;
	}

	const glm::mat4& getViewMatrix() const
	{
		if (isViewDirty)
		{
			// R^T * -T
			glm::mat3 R = glm::mat3_cast(rotation);
			glm::mat3 R_view = glm::transpose(R);
			// Construct view matrix
			view = glm::mat4(R_view);
			// Position
			view[3] = glm::vec4(R_view * (-position), 1.0f);

			isViewDirty = false;
		}
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