#pragma once

struct CameraComponent
{
	float fov = 60.0f;
	float zNear = 0.1f;
	float zFar = 1000.0f;
	float orthoSize = 15.0f;
	int bufferNubmer = -1;

	CameraComponent() = default;
};