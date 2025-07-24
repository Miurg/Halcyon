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
        _systemManager.UnsubscribeFromAll(entity);
    }

    template<typename T, typename... Args>
    T* AddComponent(Entity entity, Args&&... args)
    {
        T* component = _componentManager.AddComponent<T>(entity, std::forward<Args>(args)...);
        return component;
    }

    template<typename T>
    void RemoveComponent(Entity entity)
    {
        _componentManager.RemoveComponent<T>(entity);
	   _systemManager.CheckEntitySubscriptions(entity, _componentManager);
    }

    template<typename T>
    T* GetComponent(Entity entity)
    {
        return _componentManager.GetComponent<T>(entity);
    }

    template<typename T, typename... Args>
    void AddSystemToManager(Args&&... args)
    {
        _systemManager.AddSystem<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    void SubscribeEntity(Entity entity)
    {
        _systemManager.Subscribe<T>(entity, _componentManager);
    }

    template<typename T>
    void UnsubscribeEntity(Entity entity)
    {
        _systemManager.Unsubscribe<T>(entity);
    }


    void Update(float deltaTime) 
    {
        _systemManager.UpdateSystems(deltaTime, _componentManager);
    }
    const std::unordered_set<Entity>& GetActiveEntities() const { return _activeEntities; }
};