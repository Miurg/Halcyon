#pragma once
#include "../../Core/Components/Components.h"
#include "../../Core/Systems/System.h"

class MovementSystem : public System<MovementSystem, TransformComponent, VelocityComponent>
{
public:
	void ProcessEntity(Entity entity, ComponentManager& cm, float deltaTime) override
	{
		TransformComponent* transform = cm.GetComponent<TransformComponent>(entity);
		VelocityComponent* velocity = cm.GetComponent<VelocityComponent>(entity);

		transform->SetPosition(transform->GetPosition() + velocity->Velocity * deltaTime);
	}
};