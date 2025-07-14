#pragma once

#include <memory>
#include <vector>
#include "Components/ComponentManager.h"
#include "Systems/System.h"
#include <unordered_set>
#include "Entitys/EntityManager.h"
#include "Systems/SystemManager.h"

class ECSManager 
{
private:
    ComponentManager _componentManager;
    EntityManager _entityManager;
    SystemManager _systemManager;
    std::unordered_set<Entity> _activeEntities;

public:
    Entity CreateEntity()
    {
	   Entity entity = _entityManager.CreateEntity();
	   _activeEntities.insert(entity);
	   return entity;
    }

    void DestroyEntity(Entity entity)
    {
        _componentManager.RemoveEntity(entity);
        _activeEntities.erase(entity);
        _systemManager.UnsubscribeFromAllSystems(entity);
    }

    template<typename T, typename... Args>
    T* AddComponentToEntity(Entity entity, Args&&... args)
    {
        T* component = _componentManager.AddComponent<T>(entity, std::forward<Args>(args)...);
        return component;
    }

    template<typename T>
    void RemoveComponentFromEntity(Entity entity)
    {
        _componentManager.RemoveComponent<T>(entity);
        for (auto& system : _systemManager.Systems)
        {
            if (!system->ShouldProceaw6ssEntity(entity, _componentManager))
            {
                system->UnsubscribeEntity(entity);
                std::cerr << "Entity " << entity << " unsubscribed from system " 
                         << typeid(*system).name() << " because it doesn't have all required components" << std::endl;
            }
        }
    }

    template<typename T>
    T* GetComponentFromEntity(Entity entity)
    {
        return _componentManager.GetComponent<T>(entity);
    }

    template<typename T, typename... Args>
    void AddSystemToECSManager(Args&&... args)
    {
        _systemManager.AddSystem<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    void SubscribeEntityToSystem(Entity entity)
    {
        _systemManager.Subscribe<T>(entity, _componentManager);
    }

    template<typename T>
    void UnsubscribeEntityFromSystem(Entity entity)
    {
        _systemManager.Unsubscribe<T>(entity);
    }


    void Update(float deltaTime) 
    {
        _systemManager.UpdateSystems(deltaTime, _componentManager);
    }
    const std::unordered_set<Entity>& GetActiveEntities() const { return _activeEntities; }
};