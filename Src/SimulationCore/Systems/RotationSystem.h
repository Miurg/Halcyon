#pragma once
#include <omp.h>

#include "../../Core/Systems/SystemSubscribed.h"
#include "../Components/RotationSpeedComponent.h"
class RotationSystem : public SystemSubscribed<RotationSystem, TransformComponent, RotationSpeedComponent>
{
public:
	void ProcessEntity(Entity entity, GeneralManager& gm, float deltaTime) override
	{
		TransformComponent* transform = gm.GetComponent<TransformComponent>(entity);
		RotationSpeedComponent* rotationSpeed = gm.GetComponent<RotationSpeedComponent>(entity);

		transform->RotateAroundAxis(deltaTime * rotationSpeed->Speed, rotationSpeed->Axis);
	}
};
