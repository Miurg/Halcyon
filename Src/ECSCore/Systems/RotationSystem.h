#pragma once
#include "System.h"
#include "../Components/Components.h"
#include "../TransformUtils.h"

class RotationSystem : public System<RotationSystem,TransformComponent, RotationSpeedComponent>
{
public:
    void ProcessEntity(Entity entity, ComponentManager& cm, float deltaTime) override 
    {
        TransformComponent* transform = cm.GetComponent<TransformComponent>(entity);
        RotationSpeedComponent* rotationSpeed = cm.GetComponent<RotationSpeedComponent>(entity);
        
        if (!transform || !rotationSpeed)
        {
            throw std::runtime_error("ERROR::ROTATION SYSTEM::Entity " + std::to_string(entity) + " has a missing components!");
        }
        TransformUtils::RotateAroundAxis(*transform,
            deltaTime * rotationSpeed->Speed,
            rotationSpeed->Axis);
    }
};

