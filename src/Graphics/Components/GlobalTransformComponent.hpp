#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

class TransformSystem;

struct GlobalTransformComponent
{
public:
	GlobalTransformComponent() = default;

	GlobalTransformComponent(glm::vec3 pos) : _globalPosition(pos)
	{
		_updateDirectionVectors();
	}

	// Eulers
	GlobalTransformComponent(glm::vec3 pos, glm::vec3 rotEuler)
	    : _globalPosition(pos), _globalRotation(glm::quat(glm::radians(rotEuler)))
	{
		_updateDirectionVectors();
	}

	GlobalTransformComponent(glm::vec3 pos, glm::quat rot) : _globalPosition(pos), _globalRotation(rot)
	{
		_updateDirectionVectors();
	}

	GlobalTransformComponent(glm::vec3 pos, glm::vec3 rotEuler, glm::vec3 scl)
	    : _globalPosition(pos), _globalRotation(glm::quat(glm::radians(rotEuler))), _globalScale(scl)
	{
		_updateDirectionVectors();
	}

	GlobalTransformComponent(glm::vec3 pos, glm::quat rot, glm::vec3 scl)
	    : _globalPosition(pos), _globalRotation(rot), _globalScale(scl)
	{
		_updateDirectionVectors();
	}

	GlobalTransformComponent(float x, float y, float z) : _globalPosition(glm::vec3(x, y, z))
	{
		_updateDirectionVectors();
	}

	GlobalTransformComponent(float px, float py, float pz, float rx, float ry, float rz)
	    : _globalPosition(glm::vec3(px, py, pz)), _globalRotation(glm::quat(glm::radians(glm::vec3(rx, ry, rz))))
	{
		_updateDirectionVectors();
	}

	GlobalTransformComponent(float px, float py, float pz, float rx, float ry, float rz, float sx, float sy, float sz)
	    : _globalPosition(glm::vec3(px, py, pz)), _globalRotation(glm::quat(glm::radians(glm::vec3(rx, ry, rz)))),
	      _globalScale(glm::vec3(sx, sy, sz))
	{
		_updateDirectionVectors();
	}

	GlobalTransformComponent(const GlobalTransformComponent& other)
	    : _globalPosition(other._globalPosition), _globalRotation(other._globalRotation),
	      _globalScale(other._globalScale)
	{
		_updateDirectionVectors();
	}

	// === Getters === 

	const glm::vec3& getGlobalPosition() const
	{
		return _globalPosition;
	}
	const glm::quat& getGlobalRotation() const
	{
		return _globalRotation;
	}
	const glm::vec3& getGlobalScale() const
	{
		return _globalScale;
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

	const glm::mat4& getGlobalModelMatrix() const
	{
		if (_isModelDirty)
		{
			const glm::mat3 R = glm::mat3_cast(_globalRotation);
			_globalModel[0] = glm::vec4(R[0] * _globalScale.x, 0.0f);
			_globalModel[1] = glm::vec4(R[1] * _globalScale.y, 0.0f);
			_globalModel[2] = glm::vec4(R[2] * _globalScale.z, 0.0f);
			_globalModel[3] = glm::vec4(_globalPosition, 1.0f);
			_isModelDirty = false;
		}
		return _globalModel;
	}

	const glm::mat4& getViewMatrix() const
	{
		if (_isViewDirty)
		{
			glm::mat3 R = glm::mat3_cast(_globalRotation);
			glm::mat3 R_view = glm::transpose(R);
			_view = glm::mat4(R_view);
			_view[3] = glm::vec4(R_view * (-_globalPosition), 1.0f);
			_isViewDirty = false;
		}
		return _view;
	}

	// === Pending mutations ===

	void setGlobalPosition(const glm::vec3& pos)
	{
		_pendingPositionDelta = pos - _globalPosition;
		_wasExternallyModified = true;
	}

	void moveGlobalPosition(const glm::vec3& delta)
	{
		_pendingPositionDelta += delta;
		_wasExternallyModified = true;
	}

	void setGlobalRotation(const glm::quat& rot)
	{
		_pendingRotationDelta = glm::normalize(glm::inverse(_globalRotation) * rot);
		_wasExternallyModified = true;
	}

	void setGlobalRotation(const glm::vec3& eulerDeg)
	{
		setGlobalRotation(glm::quat(glm::radians(eulerDeg)));
	}

	// Accumulates as rot = rot * q
	void rotateLocal(float angle, const glm::vec3& axis)
	{
		glm::quat q = glm::angleAxis(angle, glm::normalize(axis));
		_pendingRotationDelta = glm::normalize(_pendingRotationDelta * q);
		_wasExternallyModified = true;
	}

	// Converts to local-space delta lq = R^-1 * q * R
	void rotateGlobal(float angle, const glm::vec3& axis)
	{
		glm::quat q = glm::angleAxis(angle, glm::normalize(axis));
		glm::quat lq = glm::normalize(glm::inverse(_globalRotation) * q * _globalRotation);
		_pendingRotationDelta = glm::normalize(_pendingRotationDelta * lq);
		_wasExternallyModified = true;
	}

	void setGlobalScale(const glm::vec3& scale)
	{
		_pendingScale = scale;
		_hasPendingScale = true;
		_wasExternallyModified = true;
	}

private:

	glm::vec3 _globalPosition = {0.0f, 0.0f, 0.0f};
	glm::quat _globalRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 _globalScale = {1.0f, 1.0f, 1.0f};

	glm::vec3 _front = {1.0f, 0.0f, 0.0f};
	glm::vec3 _up = {0.0f, 1.0f, 0.0f};
	glm::vec3 _right = {0.0f, 0.0f, 1.0f};

	mutable glm::mat4 _globalModel = glm::mat4(1.0f);
	mutable glm::mat4 _view = glm::mat4(1.0f);
	mutable bool _isModelDirty = true;
	mutable bool _isViewDirty = true;

	// == Deltas ===
	glm::vec3 _pendingPositionDelta = {0.0f, 0.0f, 0.0f};
	glm::quat _pendingRotationDelta = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	bool _hasPendingScale = false;
	glm::vec3 _pendingScale = {1.0f, 1.0f, 1.0f};

	bool _wasExternallyModified = false;

	void _updateDirectionVectors()
	{
		_front = glm::rotate(_globalRotation, glm::vec3(0.0f, 0.0f, -1.0f));
		_up = glm::rotate(_globalRotation, glm::vec3(0.0f, 1.0f, 0.0f));
		_right = glm::rotate(_globalRotation, glm::vec3(1.0f, 0.0f, 0.0f));
		_isViewDirty = true;
	}

	// Called only by TransformSystem - does NOT set _wasExternallyModified
	void _applyTransform(const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scale)
	{
		_globalPosition = pos;
		_globalRotation = rot;
		_globalScale = scale;
	}

	void _clearPending()
	{
		_pendingPositionDelta = {0.0f, 0.0f, 0.0f};
		_pendingRotationDelta = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		_hasPendingScale = false;
		_wasExternallyModified = false;
		// _isModelDirty and _isViewDirty stay true — rendering needs to recompute matrices
	}

	friend class TransformSystem;
};
