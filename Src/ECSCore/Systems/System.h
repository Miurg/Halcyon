#pragma once
#include "../Components/ComponentManager.h"
#include <vector>

class System
{
public:
    virtual ~System() = default;
    
    virtual void Update(float deltaTime, ComponentManager& cm, const std::vector<Entity>& entities)
    {
        for (Entity entity : entities) 
        {
            ProcessEntity(entity, cm, deltaTime);
        }
    }
    
    virtual void ProcessEntity(Entity entity, ComponentManager& cm, float deltaTime) = 0;
    
    virtual bool ShouldProcessEntity(Entity entity, ComponentManager& cm) = 0;
}; 