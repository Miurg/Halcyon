#pragma once
#include <GLFW/glfw3.h>
 
#include "../../Core/Systems/SystemSubscribed.h"
#include "../../RenderCore/Camera.h"
#include "../../RenderCore/Components/CameraComponent.h"

class CameraSystem : public SystemSubscribed<CameraSystem, CameraComponent>
{
private:
	bool* _keys;

public:
	CameraSystem(bool* keyArray) : _keys(keyArray) {}

	void ProcessEntity(Entity entity, GeneralManager& gm, float deltaTime) override
	{
		CameraComponent* cameraComp = gm.GetComponent<CameraComponent>(entity);

		if (cameraComp && cameraComp->Cam)
		{
			Camera* camera = cameraComp->Cam;
			if (_keys[GLFW_KEY_W]) camera->ProcessKeyboard(Camera_Movement::FORWARD, deltaTime);
			if (_keys[GLFW_KEY_S]) camera->ProcessKeyboard(Camera_Movement::BACKWARD, deltaTime);
			if (_keys[GLFW_KEY_A]) camera->ProcessKeyboard(Camera_Movement::LEFT, deltaTime);
			if (_keys[GLFW_KEY_D]) camera->ProcessKeyboard(Camera_Movement::RIGHT, deltaTime);
		}
	}
};