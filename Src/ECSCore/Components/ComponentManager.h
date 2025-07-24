#pragma once
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include "../Entitys/EntityManager.h"

class ComponentManager; // Forward declaration

struct Component 
{
    virtual ~Component() = default;
};

class ComponentManager 
{
private:
    std::unordered_map<std::type_index, std::unordered_map<Entity, std::unique_ptr<Component>>> _components;

public:

    template<typename T, typename... Args>
    T* AddComponent(Entity entity, Args&&... args) 
    {
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = component.get();
        _components[std::type_index(typeid(T))][entity] = std::move(component);
        return ptr;
    }

    template<typename T>
    T* GetComponent(Entity entity) 
    {
        auto typeIt = _components.find(std::type_index(typeid(T)));
        if (typeIt != _components.end()) 
        {
            auto entityIt = typeIt->second.find(entity);
            if (entityIt != typeIt->second.end()) 
            {
                return static_cast<T*>(entityIt->second.get());
            }
        }
        return nullptr;
    }

    template<typename T>
    std::vector<std::pair<Entity, T*>> GetAllComponents() 
    {
        std::vector<std::pair<Entity, T*>> result;
        auto typeIt = _components.find(std::type_index(typeid(T)));
        if (typeIt != _components.end()) 
        {
            for (auto& pair : typeIt->second)
            {
                Entity entity = pair.first;
                auto& component = pair.second;
                result.emplace_back(entity, static_cast<T*>(component.get()));
            }
        }
        return result;
    }

    void RemoveEntity(Entity entity) 
    {
        for (auto& pair : _components)
        {
            pair.second.erase(entity);
        }
    }

    template<typename T>
    void RemoveComponent(Entity entity)
    {
        auto typeIt = _components.find(std::type_index(typeid(T)));
        if (typeIt != _components.end())
        {
            typeIt->second.erase(entity);
            std::cout << "Remove component" << typeid(T).name() << " at entity " << entity << std::endl;
        }
    }

};
