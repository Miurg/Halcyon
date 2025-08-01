#pragma once
#include "../../Core/Components/Components.h"
#include "../../Core/Systems/System.h"
#include "../../RenderCore/TransformUtils.h"

class RotationSystem : public System<RotationSystem, TransformComponent, RotationSpeedComponent>
{
public:
	void ProcessEntity(Entity entity, ComponentManager& cm, float deltaTime) override
	{
		TransformComponent* transform = cm.GetComponent<TransformComponent>(entity);
		RotationSpeedComponent* rotationSpeed = cm.GetComponent<RotationSpeedComponent>(entity);

		TransformUtils::RotateAroundAxis(*transform, deltaTime * rotationSpeed->Speed, rotationSpeed->Axis);
	}
};
