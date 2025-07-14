#pragma once
#include "System.h"
#include "../Components/Components.h"
#include "../TransformUtils.h"

class RotationSystem : public System 
{
public:
    void ProcessEntity(Entity entity, ComponentManager& cm, float deltaTime) override 
    {
        TransformComponent* transform = cm.GetComponent<TransformComponent>(entity);
        RotationSpeedComponent* rotationSpeed = cm.GetComponent<RotationSpeedComponent>(entity);
        
        if (!transform || !rotationSpeed)
        {
            throw std::runtime_error("ROTATION SYSTEM ERROR: at entity " + std::to_string(entity) + " missing components!");
        }
        TransformUtils::RotateAroundAxis(*transform,
            deltaTime * rotationSpeed->Speed,
            rotationSpeed->Axis);
    }

    bool ShouldProcessEntity(Entity entity, ComponentManager& cm) override 
    {
        return cm.GetComponent<TransformComponent>(entity) != nullptr && 
               cm.GetComponent<RotationSpeedComponent>(entity) != nullptr;
    }
};

