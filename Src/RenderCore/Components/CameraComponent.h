#pragma once
#include "../../RenderCore/Camera.h"
#include "../../Core/Components/ComponentManager.h"

struct CameraComponent : Component
{
	Camera* CameraInstance;
	CameraComponent(Camera* cam) : CameraInstance(cam) {}
};