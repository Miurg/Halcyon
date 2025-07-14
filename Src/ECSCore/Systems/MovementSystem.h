#pragma once
#include "System.h"
#include "../Components/Components.h"

class MovementSystem : public System
{
public:
    void ProcessEntity(Entity entity, ComponentManager& cm, float deltaTime) override
    {
        TransformComponent* transform = cm.GetComponent<TransformComponent>(entity);
        VelocityComponent* velocity = cm.GetComponent<VelocityComponent>(entity);

        if (!transform || !velocity)
        {
            throw std::runtime_error("MOVEMENT SYSTEM ERROR: at entity " + std::to_string(entity) + " missing components!");
        }
        transform->Position += velocity->Velocity * deltaTime;
    }

    bool ShouldProcessEntity(Entity entity, ComponentManager& cm) override
    {
        return cm.GetComponent<TransformComponent>(entity) != nullptr &&
            cm.GetComponent<VelocityComponent>(entity) != nullptr;
    }
};