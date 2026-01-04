#pragma once
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp> 
#include <glm/gtx/quaternion.hpp>
#include "../../Core/Components/ComponentManager.hpp"
#include "../VulkanDevice.hpp"
#include "../VulkanConst.hpp"
#include "../VulkanUtils.hpp"

struct CameraComponent
{
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;
	glm::vec3 worldUp;

	float movementSpeed;
	float mouseSensitivity;
	float fov;
	int descriptorNumber;

	CameraComponent(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
	                float yaw = -90.0f, float pitch = 0.0f)
	    : position(position), worldUp(up), movementSpeed(5.0f), mouseSensitivity(0.8f), fov(60.0f)
	{
		glm::vec3 eulerAngles(glm::radians(pitch), glm::radians(yaw), 0.0f);
		rotation = glm::quat(eulerAngles);

		updateCameraVectors();
	}

	glm::mat4 getViewMatrix()
	{
		// return glm::lookAt(position, position + front, up);
		glm::mat4 rotate = glm::mat4_cast(glm::conjugate(rotation));
		glm::mat4 translate = glm::translate(glm::mat4(1.0f), -position);

		return rotate * translate;
	}

	void updateCameraVectors()
	{
		front = glm::rotate(rotation, glm::vec3(0.0f, 0.0f, -1.0f));
		up = glm::rotate(rotation, glm::vec3(0.0f, 1.0f, 0.0f));
		right = glm::rotate(rotation, glm::vec3(1.0f, 0.0f, 0.0f));
	}

	void rotate(float xoffset, float yoffset, bool constrainPitch = true)
	{
		xoffset *= mouseSensitivity;
		yoffset *= mouseSensitivity;

		// Local y
		glm::quat qYaw = glm::angleAxis(glm::radians(-xoffset), glm::vec3(0.0f, 1.0f, 0.0f));

		// Local x
		glm::quat qPitch = glm::angleAxis(glm::radians(yoffset), glm::vec3(1.0f, 0.0f, 0.0f));

		rotation = qYaw * rotation * qPitch;
		rotation = glm::normalize(rotation);

		updateCameraVectors();
	}
};