#pragma once
#include <cmath>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "../../Core/Components/ComponentManager.h"

const GLfloat SPEED = 5.0f;
const GLfloat SENSITIVITY = 0.8f;
const GLfloat FOV = 60.0f;
const GLfloat YAW = 45.0f;
const GLfloat PITCH = 30.0f;

struct CameraComponent : Component
{
	glm::vec3 Position;
	glm::quat Rotation;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;
	GLfloat MovementSpeed;
	GLfloat MouseSensitivity;
	GLfloat Fov;
	float Yaw;
	float Pitch;
	CameraComponent(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
	                float yaw = YAW, float pitch = PITCH)
	    : Position(position), WorldUp(up), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Fov(FOV), Yaw(yaw),
	      Pitch(pitch)
	{
		UpdateCameraVectors();
	}

	glm::mat4 GetViewMatrix()
	{
		return glm::lookAt(Position, Position + Front, Up);
	}
	void UpdateCameraVectors()
	{
		float yawRad = glm::radians(Yaw);
		float pitchRad = glm::radians(Pitch);

		glm::vec3 front;
		front.x = cos(pitchRad) * sin(yawRad);
		front.y = sin(pitchRad);
		front.z = -cos(pitchRad) * cos(yawRad);
		Front = glm::normalize(front);

		Rotation = glm::quat(glm::vec3(pitchRad, yawRad, 0.0f));

		Right = glm::normalize(glm::cross(Front, WorldUp));
		Up = glm::normalize(glm::cross(Right, Front));
	}
	void ResetCamera()
	{
		Position = glm::vec3(10.0f, 10.0f, 10.0f);
		Yaw = YAW;
		Pitch = PITCH;
		UpdateCameraVectors();
	}
};

