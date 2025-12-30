#pragma once
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "../../Core/Components/ComponentManager.hpp"

#include "../DescriptorHandler.hpp"

const float SPEED = 5.0f;
const float SENSITIVITY = 0.8f;
const float FOV = 60.0f;
const float YAW = -90.0f;
const float PITCH = 30.0f;

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
	float yaw;
	float pitch;
	std::vector<vk::raii::Buffer> cameraBuffers;
	std::vector<vk::raii::DeviceMemory> cameraBuffersMemory;
	std::vector<void*> cameraBuffersMapped;
	std::vector<vk::raii::DescriptorSet> cameraDescriptorSets;

	CameraComponent(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
	                float yaw = YAW, float pitch = PITCH)
	    : position(position), worldUp(up), movementSpeed(SPEED), mouseSensitivity(SENSITIVITY), fov(FOV), yaw(yaw),
	      pitch(pitch)
	{
		updateCameraVectors();
	}

	glm::mat4 getViewMatrix()
	{
		return glm::lookAt(position, position + front, up);
	}
	void updateCameraVectors()
	{
		glm::vec3 newFront;

		newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		newFront.y = sin(glm::radians(pitch));
		newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

		front = glm::normalize(newFront);

		right = glm::normalize(glm::cross(front, worldUp));
		up = glm::normalize(glm::cross(right, front));
	}
	void resetCamera()
	{
		position = glm::vec3(10.0f, 10.0f, 10.0f);
		yaw = YAW;
		pitch = PITCH;
		updateCameraVectors();
	}
	static void initCameraBuffers(CameraComponent& camera, VulkanDevice& vulkanDevice)
	{
		camera.cameraBuffers.clear();
		camera.cameraBuffersMemory.clear();
		camera.cameraBuffersMapped.clear();

		vk::DeviceSize bufferSize = sizeof(CameraUBO);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			auto [buffer, bufferMem] = VulkanUtils::createBuffer(
			    bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
			    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vulkanDevice);

			camera.cameraBuffers.emplace_back(std::move(buffer));
			camera.cameraBuffersMemory.emplace_back(std::move(bufferMem));
			camera.cameraBuffersMapped.emplace_back(camera.cameraBuffersMemory[i].mapMemory(0, bufferSize));
		}
	}
};