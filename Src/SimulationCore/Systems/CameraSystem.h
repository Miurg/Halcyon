#pragma once
#include <GLFW/glfw3.h>
 
#include "../../Core/Systems/SystemSubscribed.h"
#include "../../RenderCore/Camera.h"
#include "../../RenderCore/Components/CameraComponent.h"
#include "../../GLFWCore/Components/KeyboardStateComponent.h"
#include "../../GLFWCore/Contexts/InputDataContext.h"

class CameraSystem : public SystemSubscribed<CameraSystem, CameraComponent>
{
private:
	bool* _keys;

public:
	CameraSystem(bool* keyArray) : _keys(keyArray) {}

	void ProcessEntity(Entity entity, GeneralManager& gm, float deltaTime) override
	{
		CameraComponent* cameraComp = gm.GetComponent<CameraComponent>(entity);
		KeyboardStateComponent* keyboardState =
		    gm.GetComponent<KeyboardStateComponent>(gm.GetContext<InputDataContext>()->InputInstance);

		if (cameraComp && cameraComp->Cam)
		{
			
			Camera* camera = cameraComp->Cam;
			if (keyboardState->Keys[GLFW_KEY_W]) camera->ProcessKeyboard(Camera_Movement::FORWARD, deltaTime);
			if (keyboardState->Keys[GLFW_KEY_S])
				camera->ProcessKeyboard(Camera_Movement::BACKWARD, deltaTime);
			if (keyboardState->Keys[GLFW_KEY_A])
				camera->ProcessKeyboard(Camera_Movement::LEFT, deltaTime);
			if (keyboardState->Keys[GLFW_KEY_D])
				camera->ProcessKeyboard(Camera_Movement::RIGHT, deltaTime);
		}
	}
};