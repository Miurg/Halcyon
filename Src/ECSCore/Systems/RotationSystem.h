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
            throw std::runtime_error("ERROR::ROTATION SYSTEM::Entity " + std::to_string(entity) + " has a missing components!");
        }
        TransformUtils::RotateAroundAxis(*transform,
            deltaTime * rotationSpeed->Speed,
            rotationSpeed->Axis);
    }

    bool ShouldProcessEntity(Entity entity, ComponentManager& cm) override 
    {
        if (cm.GetComponent<TransformComponent>(entity) == nullptr)
        {
		   std::cerr << "WARNING::ROTATION SYSTEM::Entity " << entity << " should not be processed by "
		             << typeid(*this).name() 
                     << " because it doesn't have required component: " 
                     <<  typeid(TransformComponent).name() 
                     << std::endl;
        }
	    if (cm.GetComponent<RotationSpeedComponent>(entity) == nullptr)
        {
		   std::cerr << "WARNING::ROTATION SYSTEM::Entity " << entity << " should not be processed by "
		             << typeid(*this).name()
		             << " because it doesn't have required component: " 
                     << typeid(RotationSpeedComponent).name()
		             << std::endl;
        }
       return cm.GetComponent<TransformComponent>(entity) != nullptr && 
               cm.GetComponent<RotationSpeedComponent>(entity) != nullptr;
    }
};

