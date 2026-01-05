#pragma once

struct CameraComponent
{
	float fov = 60.0f;
	int descriptorNumber = -1;

	CameraComponent() = default;

	CameraComponent(float fov)
	    : fov(fov)
	{}

};