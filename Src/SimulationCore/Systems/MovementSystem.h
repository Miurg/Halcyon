#pragma once
#include "../../Core/Systems/System.h"
#include "../../Core/Components/Components.h"

class MovementSystem : public System<MovementSystem,TransformComponent, VelocityComponent>
{
public:
    void ProcessEntity(Entity entity, ComponentManager& cm, float deltaTime) override
    {
        TransformComponent* transform = cm.GetComponent<TransformComponent>(entity);
        VelocityComponent* velocity = cm.GetComponent<VelocityComponent>(entity);

        transform->Position += velocity->Velocity * deltaTime;
    }
};