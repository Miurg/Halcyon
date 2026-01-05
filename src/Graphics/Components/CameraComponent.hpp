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
	float movementSpeed = 5.0f;
	float mouseSensitivity = 0.8f;
	float fov = 60.0f;
	int descriptorNumber = -1;

	CameraComponent() = default;

	CameraComponent(float movementSpeed, float mouseSensitivity, float fov)
	    : movementSpeed(movementSpeed), mouseSensitivity(mouseSensitivity), fov(fov)
	{}

};