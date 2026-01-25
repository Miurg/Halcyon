#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp> 
#include <glm/gtx/quaternion.hpp>

struct GlobalTransformComponent
{
	glm::vec3 globalPosition = {0.0f, 0.0f, 0.0f};
	glm::quat globalRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); 
	glm::vec3 globalScale = {1.0f, 1.0f, 1.0f};

	glm::vec3 front = {1.0f, 0.0f, 0.0f};
	glm::vec3 up = {0.0f, 1.0f, 0.0f};
	glm::vec3 right = {0.0f, 0.0f, 1.0f};

	mutable glm::mat4 globalModel = glm::mat4(1.0f); 
	mutable glm::mat4 view = glm::mat4(1.0f); 
	mutable bool isModelDirty = true;
	mutable bool isViewDirty = true;

	GlobalTransformComponent() = default;

	GlobalTransformComponent(glm::vec3 pos) : globalPosition(pos) 
	{
		updateDirectionVectors();
	}

	//Eulers
	GlobalTransformComponent(glm::vec3 pos, glm::vec3 rotEuler) : globalPosition(pos), globalRotation(glm::quat(glm::radians(rotEuler))) 
	{
		updateDirectionVectors();
	}

	GlobalTransformComponent(glm::vec3 pos, glm::quat rot) : globalPosition(pos), globalRotation(rot) 
	{
		updateDirectionVectors();
	}

	GlobalTransformComponent(glm::vec3 pos, glm::vec3 rotEuler, glm::vec3 scl)
	    : globalPosition(pos), globalRotation(glm::quat(glm::radians(rotEuler))), globalScale(scl)
	{
		updateDirectionVectors();
	}

	GlobalTransformComponent(glm::vec3 pos, glm::quat rot, glm::vec3 scl) : globalPosition(pos), globalRotation(rot), globalScale(scl) 
	{
		updateDirectionVectors();
	}

	GlobalTransformComponent(float x, float y, float z) : globalPosition(glm::vec3(x, y, z)) 
	{
		updateDirectionVectors();
	}

	GlobalTransformComponent(float px, float py, float pz, float rx, float ry, float rz)
	    : globalPosition(glm::vec3(px, py, pz)), globalRotation(glm::quat(glm::radians(glm::vec3(rx, ry, rz))))
	{
		updateDirectionVectors();
	}

	GlobalTransformComponent(float px, float py, float pz, float rx, float ry, float rz, float sx, float sy, float sz)
	    : globalPosition(glm::vec3(px, py, pz)), globalRotation(glm::quat(glm::radians(glm::vec3(rx, ry, rz)))),
	      globalScale(glm::vec3(sx, sy, sz))
	{
		updateDirectionVectors();
	}

	GlobalTransformComponent(const GlobalTransformComponent& other)
	    : globalPosition(other.globalPosition), globalRotation(other.globalRotation), globalScale(other.globalScale)
	{
		updateDirectionVectors();
	}

	void updateDirectionVectors()
	{
		front = glm::rotate(globalRotation, glm::vec3(0.0f, 0.0f, -1.0f));
		up = glm::rotate(globalRotation, glm::vec3(0.0f, 1.0f, 0.0f));
		right = glm::rotate(globalRotation, glm::vec3(1.0f, 0.0f, 0.0f));
		isViewDirty = true;
	}

	void computeGlobalModelMatrix()
	{
		// R * S + T
		const glm::mat3 rotationMatrix = glm::mat3_cast(globalRotation);
		// X axis
		globalModel[0] = glm::vec4(rotationMatrix[0] * globalScale.x, 0.0f);
		// Y axis
		globalModel[1] = glm::vec4(rotationMatrix[1] * globalScale.y, 0.0f);
		// Z axis
		globalModel[2] = glm::vec4(rotationMatrix[2] * globalScale.z, 0.0f);
		// Position
		globalModel[3] = glm::vec4(globalPosition, 1.0f);
	}

	void computeGlobalTransform(glm::vec3 finalPosition, glm::quat finalRotation, glm::vec3 finalScale) 
	{
		globalScale = finalScale;
		globalRotation = finalRotation;
		globalPosition = finalPosition;
	}

	const glm::mat4& getGlobalModelMatrix() const
	{
		if (isModelDirty)
		{
			// R * S + T
			const glm::mat3 rotationMatrix = glm::mat3_cast(globalRotation);
			// X axis
			globalModel[0] = glm::vec4(rotationMatrix[0] * globalScale.x, 0.0f);
			// Y axis
			globalModel[1] = glm::vec4(rotationMatrix[1] * globalScale.y, 0.0f);
			// Z axis
			globalModel[2] = glm::vec4(rotationMatrix[2] * globalScale.z, 0.0f);
			// Position
			globalModel[3] = glm::vec4(globalPosition, 1.0f);

			isModelDirty = false;
		}
		return globalModel;
	}

	const glm::mat4& getViewMatrix() const
	{
		if (isViewDirty)
		{
			// R^T * -T
			glm::mat3 R = glm::mat3_cast(globalRotation);
			glm::mat3 R_view = glm::transpose(R);
			// Construct view matrix
			view = glm::mat4(R_view);
			// Position
			view[3] = glm::vec4(R_view * (-globalPosition), 1.0f);

			isViewDirty = false;
		}
		return view;
	}

	// Local euler
	void rotateLocal(const glm::vec3& eulerAngles)
	{
		glm::quat q = glm::quat(eulerAngles);
		globalRotation = globalRotation * q;
		globalRotation = glm::normalize(globalRotation);
		updateDirectionVectors();
	}

	// Local
	void rotateLocal(float angle, const glm::vec3& axis)
	{
		glm::quat q = glm::angleAxis(angle, glm::normalize(axis));
		globalRotation = globalRotation * q;
		globalRotation = glm::normalize(globalRotation);
		updateDirectionVectors();
	}

	// Global euler
	void rotateGlobal(const glm::vec3& eulerAngles)
	{
		glm::quat q = glm::quat(eulerAngles);
		globalRotation = q * globalRotation;
		globalRotation = glm::normalize(globalRotation);
		updateDirectionVectors();
	}

	// Global
	void rotateGlobal(float angle, const glm::vec3& axis)
	{
		glm::quat q = glm::angleAxis(angle, glm::normalize(axis));
		globalRotation = q * globalRotation;
		globalRotation = glm::normalize(globalRotation);
		updateDirectionVectors();
	}
};