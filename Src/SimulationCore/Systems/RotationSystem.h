#pragma once
#include <omp.h>

#include "../../Core/Systems/System.h"
#include "../Components/RotationSpeedComponent.h"
class RotationSystem : public System<RotationSystem, TransformComponent, RotationSpeedComponent>
{
public:
	void ProcessEntity(Entity entity, ComponentManager& cm, float deltaTime) override
	{
		TransformComponent* transform = cm.GetComponent<TransformComponent>(entity);
		RotationSpeedComponent* rotationSpeed = cm.GetComponent<RotationSpeedComponent>(entity);

		transform->RotateAroundAxis(deltaTime * rotationSpeed->Speed, rotationSpeed->Axis);
	}
};
