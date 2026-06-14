#pragma once

struct CameraComponent
{
	float fov = 60.0f;
	float zNear = 0.1f;
	float zFar = 1000.0f;
	float orthoSize = 5.0f;

	CameraComponent() = default;
};