#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "../../Core/Components/ComponentManager.h"

struct TransformComponent : Component
{
private:
	glm::vec3 Position = glm::vec3(0.0f);
	glm::quat Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 Scale = glm::vec3(1.0f);
	mutable bool IsDirty = false;

public:
	TransformComponent() = default;

	TransformComponent(glm::vec3 pos) : Position(pos), IsDirty(true) {}

	TransformComponent(glm::vec3 pos, glm::quat rot) : Position(pos), Rotation(rot), IsDirty(true) {}

	TransformComponent(glm::vec3 pos, glm::quat rot, glm::vec3 scl)
	    : Position(pos), Rotation(rot), Scale(scl), IsDirty(true)
	{
	}

	inline void SetPosition(const glm::vec3& pos)
	{
		Position = pos;
		IsDirty = true;
	}
	inline void SetRotation(const glm::quat& rot)
	{
		Rotation = rot;
		IsDirty = true;
	}
	inline void SetScale(const glm::vec3& scl)
	{
		Scale = scl;
		IsDirty = true;
	}
	inline void ClearDirty() const
	{
		IsDirty = false;
	}
	inline glm::vec3 GetPosition() const
	{
		return Position;
	}
	inline glm::quat GetRotation() const
	{
		return Rotation;
	}
	inline glm::vec3 GetScale() const
	{
		return Scale;
	}
	inline bool GetDirty() const
	{
		return IsDirty;
	}

	inline glm::mat4 CreateTransformMatrix(glm::vec3 pos, glm::quat rot, glm::vec3 scl)
	{
		return glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(rot) * glm::scale(glm::mat4(1.0f), scl);
	}

	inline glm::mat4 CreateTransformMatrix(TransformComponent transform)
	{
		return CreateTransformMatrix(transform.Position, transform.Rotation, transform.Scale);
	}

	inline void SetRotationEuler(float pitch, float yaw, float roll)
	{
		Rotation = glm::quat(glm::radians(glm::vec3(pitch, yaw, roll)));
		IsDirty = true;
	}

	inline void SetRotationEuler(const glm::vec3& eulerAngles)
	{
		Rotation = glm::quat(glm::radians(eulerAngles));
		IsDirty = true;
	}

	inline glm::vec3 GetEulerAngles()
	{
		glm::vec3 euler;

		// pitch (x-axis rotation)
		float sinr_cosp = 2 * (Rotation.w * Rotation.x + Rotation.y * Rotation.z);
		float cosr_cosp = 1 - 2 * (Rotation.x * Rotation.x + Rotation.y * Rotation.y);
		euler.x = std::atan2(sinr_cosp, cosr_cosp);

		// yaw (y-axis rotation)
		float sinp = 2 * (Rotation.w * Rotation.y - Rotation.z * Rotation.x);
		if (std::abs(sinp) >= 1)
			euler.y = std::copysign(3.14159265359f / 2, sinp); // use 90 degrees if out of range
		else
			euler.y = std::asin(sinp);

		// roll (z-axis rotation)
		float siny_cosp = 2 * (Rotation.w * Rotation.z + Rotation.x * Rotation.y);
		float cosy_cosp = 1 - 2 * (Rotation.y * Rotation.y + Rotation.z * Rotation.z);
		euler.z = std::atan2(siny_cosp, cosy_cosp);

		return glm::degrees(euler);
	}

	inline void RotateAroundAxis(float angle, const glm::vec3& axis)
	{
		glm::quat rotation = glm::angleAxis(glm::radians(angle), glm::normalize(axis));
		Rotation = rotation * Rotation;
		IsDirty = true;
	}

	inline void RotateLocalX(float angle)
	{
		glm::vec3 right = glm::normalize(Rotation * glm::vec3(1, 0, 0));
		RotateAroundAxis(angle, right);
	}

	inline void RotateLocalY(float angle)
	{
		glm::vec3 up = glm::normalize(Rotation * glm::vec3(0, 1, 0));
		RotateAroundAxis(angle, up);
	}

	inline void RotateLocalZ(float angle)
	{
		glm::vec3 forward = glm::normalize(Rotation * glm::vec3(0, 0, 1));
		RotateAroundAxis(angle, forward);
	}

	inline glm::vec3 GetForward()
	{
		return glm::normalize(Rotation * glm::vec3(0, 0, 1));
	}

	inline glm::vec3 GetRight()
	{
		return glm::normalize(Rotation * glm::vec3(1, 0, 0));
	}

	inline glm::vec3 GetUp()
	{
		return glm::normalize(Rotation * glm::vec3(0, 1, 0));
	}

	inline TransformComponent CreateFromMatrix(const glm::mat4& matrix)
	{
		TransformComponent transform;
		transform.Position = glm::vec3(matrix[3]);
		glm::mat3 rotationMatrix = glm::mat3(matrix);
		transform.Rotation = glm::quat_cast(rotationMatrix);
		transform.Scale.x = glm::length(glm::vec3(matrix[0]));
		transform.Scale.y = glm::length(glm::vec3(matrix[1]));
		transform.Scale.z = glm::length(glm::vec3(matrix[2]));
		transform.IsDirty = true;
		return transform;
	}
};
