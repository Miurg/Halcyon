#pragma once
#include "../Components/ComponentManager.h"
#include <vector>
#include <unordered_map>
#include <typeindex>
#include "System.h"

class SystemManager
{
public:
    std::vector<std::unique_ptr<System>> Systems;
    std::unordered_map<std::type_index, std::vector<Entity>> SystemToEntities;
    std::unordered_map<Entity, std::vector<std::type_index>> EntityToSystems;

    template<typename T, typename... Args>
    void AddSystem(Args&&... args)
    {
        Systems.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

    template<typename T>
    void Subscribe(Entity entity, ComponentManager& cm) 
    {
        std::type_index systemType = typeid(T);
        
        // Находим систему в векторе
        System* system = nullptr;
        for (auto& sys : Systems)
        {
            if (std::type_index(typeid(*sys)) == systemType)
            {
                system = sys.get();
                break;
            }
        }
        
        if (system && system->ShouldProcessEntity(entity, cm))
        {
            SystemToEntities[systemType].push_back(entity);
            EntityToSystems[entity].push_back(systemType);
        }
        else
        {
            std::cerr << "WARNING::SYSTEM MANAGER::Entity " << entity << " does not meet the requirements for system " << systemType.name() << std::endl;
        }
    }

    template<typename T>
    void Unsubscribe(Entity entity) 
    {
        std::type_index systemType = typeid(T);
        
        auto systemIt = SystemToEntities.find(systemType);
        if (systemIt != SystemToEntities.end())
        {
            auto& entities = systemIt->second;
            auto entityIt = std::find(entities.begin(), entities.end(), entity);
		    std::cerr << "ERASE " << entity << std::endl;
            if (entityIt != entities.end())
            {
                entities.erase(entityIt);
			    std::cerr << "ERASE " << entity << std::endl;
			   
            }
        }

        auto entityIt = EntityToSystems.find(entity);
        if (entityIt != EntityToSystems.end())
        {
            auto& systems = entityIt->second;
            auto systemIt = std::find(systems.begin(), systems.end(), systemType);
            if (systemIt != systems.end())
            {
                systems.erase(systemIt);
            }
        }
    }

    void UnsubscribeFromAll(Entity entity)
    {
        auto it = EntityToSystems.find(entity);
        if (it != EntityToSystems.end())
        {
            for (const auto& systemType : it->second)
            {
                auto& entities = SystemToEntities[systemType];
                entities.erase(std::remove(entities.begin(), entities.end(), entity), entities.end());
            }
            EntityToSystems.erase(it);
        }
    }

    void CheckEntitySubscriptions(Entity entity, ComponentManager& cm)
    {
	   std::vector<std::type_index> systemsToUnsubscribe;

	   for (auto& system : Systems)
	   {
		  std::type_index systemType = typeid(*system);
		  if (!system->ShouldProcessEntity(entity, cm))
		  {
			 systemsToUnsubscribe.push_back(systemType);
		  }
	   }

	   for (const auto& systemType : systemsToUnsubscribe)
	   {
		  auto systemIt = SystemToEntities.find(systemType);
		  if (systemIt != SystemToEntities.end())
		  {
			 auto& entities = systemIt->second;
			 entities.erase(std::remove(entities.begin(), entities.end(), entity), entities.end());
		  }

		  auto entityIt = EntityToSystems.find(entity);
		  if (entityIt != EntityToSystems.end())
		  {
			 auto& systems = entityIt->second;
			 systems.erase(std::remove(systems.begin(), systems.end(), systemType), systems.end());
		  }

		  std::cerr << "WARNING::SYSTEM MANAGER::Entity " << entity << " unsubscribed from system " << systemType.name()
		            << " because it doesn't have all required components" << std::endl;
	   }
    }

    void UpdateSystems(float deltaTime, ComponentManager& cm)
    {
        for (auto& system : Systems)
        {
            std::type_index systemType = typeid(*system);
            auto it = SystemToEntities.find(systemType);
            if (it != SystemToEntities.end())
            {
                system->Update(deltaTime, cm, it->second);
            }
        }
    }
};