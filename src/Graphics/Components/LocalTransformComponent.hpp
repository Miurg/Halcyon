#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

struct LocalTransformComponent
{
	glm::vec3 localPosition = {0.0f, 0.0f, 0.0f};
	glm::quat localRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 localScale = {1.0f, 1.0f, 1.0f};

	glm::vec3 front = {1.0f, 0.0f, 0.0f};
	glm::vec3 up = {0.0f, 1.0f, 0.0f};
	glm::vec3 right = {0.0f, 0.0f, 1.0f};

	// mutable glm::mat4 localModel = glm::mat4(1.0f);
	mutable glm::mat4 view = glm::mat4(1.0f);
	mutable bool isModelDirty = true;
	mutable bool isViewDirty = true;

	LocalTransformComponent() = default;

	LocalTransformComponent(glm::vec3 pos) : localPosition(pos)
	{
		updateDirectionVectors();
	}

	// Eulers
	LocalTransformComponent(glm::vec3 pos, glm::vec3 rotEuler)
	    : localPosition(pos), localRotation(glm::quat(glm::radians(rotEuler)))
	{
		updateDirectionVectors();
	}

	LocalTransformComponent(glm::vec3 pos, glm::quat rot) : localPosition(pos), localRotation(rot)
	{
		updateDirectionVectors();
	}

	LocalTransformComponent(glm::vec3 pos, glm::vec3 rotEuler, glm::vec3 scl)
	    : localPosition(pos), localRotation(glm::quat(glm::radians(rotEuler))), localScale(scl)
	{
		updateDirectionVectors();
	}

	LocalTransformComponent(glm::vec3 pos, glm::quat rot, glm::vec3 scl)
	    : localPosition(pos), localRotation(rot), localScale(scl)
	{
		updateDirectionVectors();
	}

	LocalTransformComponent(float x, float y, float z) : localPosition(glm::vec3(x, y, z))
	{
		updateDirectionVectors();
	}

	LocalTransformComponent(float px, float py, float pz, float rx, float ry, float rz)
	    : localPosition(glm::vec3(px, py, pz)), localRotation(glm::quat(glm::radians(glm::vec3(rx, ry, rz))))
	{
		updateDirectionVectors();
	}

	LocalTransformComponent(float px, float py, float pz, float rx, float ry, float rz, float sx, float sy, float sz)
	    : localPosition(glm::vec3(px, py, pz)), localRotation(glm::quat(glm::radians(glm::vec3(rx, ry, rz)))),
	      localScale(glm::vec3(sx, sy, sz))
	{
		updateDirectionVectors();
	}

	LocalTransformComponent(const LocalTransformComponent& other)
	    : localPosition(other.localPosition), localRotation(other.localRotation), localScale(other.localScale)
	{
		updateDirectionVectors();
	}

	void updateDirectionVectors()
	{
		front = glm::rotate(localRotation, glm::vec3(0.0f, 0.0f, -1.0f));
		up = glm::rotate(localRotation, glm::vec3(0.0f, 1.0f, 0.0f));
		right = glm::rotate(localRotation, glm::vec3(1.0f, 0.0f, 0.0f));
		isModelDirty = true;
		isViewDirty = true;
	}


	const glm::mat4& getViewMatrix() const
	{
		if (isViewDirty)
		{
			// R^T * -T
			glm::mat3 R = glm::mat3_cast(localRotation);
			glm::mat3 R_view = glm::transpose(R);
			// Construct view matrix
			view = glm::mat4(R_view);
			// Position
			view[3] = glm::vec4(R_view * (-localPosition), 1.0f);

			isViewDirty = false;
		}
		return view;
	}

	// Local euler
	void rotateLocal(const glm::vec3& eulerAngles)
	{
		glm::quat q = glm::quat(eulerAngles);
		localRotation = localRotation * q;
		localRotation = glm::normalize(localRotation);
		updateDirectionVectors();
	}

	// Local
	void rotateLocal(float angle, const glm::vec3& axis)
	{
		glm::quat q = glm::angleAxis(angle, glm::normalize(axis));
		localRotation = localRotation * q;
		localRotation = glm::normalize(localRotation);
		updateDirectionVectors();
	}

	// Global euler
	void rotateGlobal(const glm::vec3& eulerAngles)
	{
		glm::quat q = glm::quat(eulerAngles);
		localRotation = q * localRotation;
		localRotation = glm::normalize(localRotation);
		updateDirectionVectors();
	}

	// Global
	void rotateGlobal(float angle, const glm::vec3& axis)
	{
		glm::quat q = glm::angleAxis(angle, glm::normalize(axis));
		localRotation = q * localRotation;
		localRotation = glm::normalize(localRotation);
		updateDirectionVectors();
	}
};