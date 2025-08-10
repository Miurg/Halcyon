#pragma once
#include "../../RenderCore/Camera.h"
#include "../../Core/Components/ComponentManager.h"

struct CameraComponent : Component
{
	Camera* Cam;
	CameraComponent(Camera* cam) : Cam(cam) {}
};