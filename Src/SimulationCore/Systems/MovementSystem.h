#pragma once
#include "../../RenderCore/Components/TransformComponent.h"
#include "../../Core/Systems/SystemSubscribed.h"
#include "../Components/VelocityComponent.h"

class MovementSystem : public SystemSubscribed<MovementSystem, TransformComponent, VelocityComponent>
{
public:
	void ProcessEntity(Entity entity, GeneralManager& gm, float deltaTime) override
	{
		TransformComponent* transform = gm.GetComponent<TransformComponent>(entity);
		VelocityComponent* velocity = gm.GetComponent<VelocityComponent>(entity);

		transform->SetPosition(transform->GetPosition() + velocity->Velocity * deltaTime);
	}
};