#pragma once

#include "HalcyonExport.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include "PRSStructure.hpp"

class TransformSystem;

struct HALCYON_API LocalTransformComponent
{
public:
	LocalTransformComponent() = default;

	LocalTransformComponent(glm::vec3 pos) : prs{pos}
	{
		_updateDirectionVectors();
	}

	// Eulers
	LocalTransformComponent(glm::vec3 pos, glm::vec3 rotEuler) : prs{pos, glm::quat(glm::radians(rotEuler))}
	{
		_updateDirectionVectors();
	}

	LocalTransformComponent(glm::vec3 pos, glm::quat rot) : prs{pos, rot}
	{
		_updateDirectionVectors();
	}

	LocalTransformComponent(glm::vec3 pos, glm::vec3 rotEuler, glm::vec3 scl)
	    : prs{pos, glm::quat(glm::radians(rotEuler)), scl}
	{
		_updateDirectionVectors();
	}

	LocalTransformComponent(glm::vec3 pos, glm::quat rot, glm::vec3 scl) : prs{pos, rot, scl}
	{
		_updateDirectionVectors();
	}

	LocalTransformComponent(float x, float y, float z) : prs{(glm::vec3(x, y, z))}
	{
		_updateDirectionVectors();
	}

	LocalTransformComponent(float px, float py, float pz, float rx, float ry, float rz)
	    : prs{glm::vec3(px, py, pz), glm::quat(glm::radians(glm::vec3(rx, ry, rz)))}
	{
		_updateDirectionVectors();
	}

	LocalTransformComponent(float px, float py, float pz, float rx, float ry, float rz, float sx, float sy, float sz)
	    : prs{glm::vec3(px, py, pz), glm::quat(glm::radians(glm::vec3(rx, ry, rz))), glm::vec3(sx, sy, sz)}
	{
		_updateDirectionVectors();
	}

	LocalTransformComponent(const LocalTransformComponent& other) : prs(other.prs)
	{
		_updateDirectionVectors();
	}

	// === Getters ===

	const glm::vec3& getLocalPosition() const
	{
		return prs.position;
	}
	const glm::quat& getLocalRotation() const
	{
		return prs.rotation;
	}
	const glm::vec3& getLocalScale() const
	{
		return prs.scale;
	}
	const glm::vec3& getFront() const
	{
		return _front;
	}
	const glm::vec3& getUp() const
	{
		return _up;
	}
	const glm::vec3& getRight() const
	{
		return _right;
	}

	const glm::mat4& getViewMatrix() const
	{
		if (_isViewDirty)
		{
			glm::mat3 R = glm::mat3_cast(prs.rotation);
			glm::mat3 R_view = glm::transpose(R);
			_view = glm::mat4(R_view);
			_view[3] = glm::vec4(R_view * (-prs.position), 1.0f);
			_isViewDirty = false;
		}
		return _view;
	}

	// === Pending mutations ===

	void setLocalPosition(const glm::vec3& pos)
	{
		_pendingPositionDelta = pos - prs.position; // delta to target, OVERWRITES
		_isModelDirty = true;
	}

	void moveLocalPosition(const glm::vec3& delta)
	{
		_pendingPositionDelta += delta; // ACCUMULATES
		_isModelDirty = true;
	}

	void setLocalRotation(const glm::quat& rot)
	{
		_pendingRotationDelta = glm::normalize(glm::inverse(prs.rotation) * rot); // OVERWRITES
		_isModelDirty = true;
	}

	void setLocalRotation(const glm::vec3& eulerDeg)
	{
		setLocalRotation(glm::quat(glm::radians(eulerDeg)));
	}

	// Accumulates as rot = rot * q
	void rotateLocal(float angle, const glm::vec3& axis)
	{
		glm::quat q = glm::angleAxis(angle, glm::normalize(axis));
		_pendingRotationDelta = glm::normalize(_pendingRotationDelta * q);
		_isModelDirty = true;
	}

	// Converts to local-space delta lq = R^-1 * q * R
	void rotateGlobal(float angle, const glm::vec3& axis)
	{
		glm::quat q = glm::angleAxis(angle, glm::normalize(axis));
		glm::quat lq = glm::normalize(glm::inverse(prs.rotation) * q * prs.rotation);
		_pendingRotationDelta = glm::normalize(_pendingRotationDelta * lq);
		_isModelDirty = true;
	}

	void setLocalScale(const glm::vec3& scale)
	{
		_pendingScale = scale;
		_hasPendingScale = true;
		_isModelDirty = true;
	}

private:

	PRS prs;

	glm::vec3 _front = {1.0f, 0.0f, 0.0f};
	glm::vec3 _up = {0.0f, 1.0f, 0.0f};
	glm::vec3 _right = {0.0f, 0.0f, 1.0f};

	mutable glm::mat4 _view = glm::mat4(1.0f);
	mutable bool _isModelDirty = true;
	mutable bool _isViewDirty = true;

	// == Deltas ===

	glm::vec3 _pendingPositionDelta = {0.0f, 0.0f, 0.0f};
	glm::quat _pendingRotationDelta = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	bool _hasPendingScale = false;
	glm::vec3 _pendingScale = {1.0f, 1.0f, 1.0f};

	void _updateDirectionVectors()
	{
		_front = prs.rotation * glm::vec3(0.0f, 0.0f, -1.0f);
		_up = prs.rotation * glm::vec3(0.0f, 1.0f, 0.0f);
		_right = prs.rotation * glm::vec3(1.0f, 0.0f, 0.0f);
		_isModelDirty = true;
		_isViewDirty = true;
	}

	void _clearPending()
	{
		_pendingPositionDelta = {0.0f, 0.0f, 0.0f};
		_pendingRotationDelta = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		_hasPendingScale = false;
		_isModelDirty = false;
	}

	friend class TransformSystem;
};
