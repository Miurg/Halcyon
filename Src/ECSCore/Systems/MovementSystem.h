#pragma once
#include "System.h"
#include "../Components/Components.h"

class MovementSystem : public System<MovementSystem,TransformComponent, VelocityComponent>
{
public:
    void ProcessEntity(Entity entity, ComponentManager& cm, float deltaTime) override
    {
        TransformComponent* transform = cm.GetComponent<TransformComponent>(entity);
        VelocityComponent* velocity = cm.GetComponent<VelocityComponent>(entity);

        if (!transform || !velocity)
        {
            throw std::runtime_error("ERROR::MOVEMENT SYSTEM::Entity " + std::to_string(entity) + " has a missing components!");
        }
        transform->Position += velocity->Velocity * deltaTime;
    }
};