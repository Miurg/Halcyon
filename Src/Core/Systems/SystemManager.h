#pragma once
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "../Components/ComponentManager.h"
#include "System.h"

class SystemManager
{
public:
	std::vector<std::unique_ptr<SystemBase>> Systems;
	std::unordered_map<std::type_index, std::vector<Entity>> SystemToEntities;
	std::unordered_map<Entity, std::vector<std::type_index>> EntityToSystems;

	template <typename TSystem, typename... Args>
	void AddSystem(Args&&... args)
	{
		Systems.push_back(std::make_unique<TSystem>(std::forward<Args>(args)...));
	}

	template <typename TSystem>
	void Subscribe(Entity entity, ComponentManager& cm)
	{
		std::type_index systemType = typeid(TSystem);

		SystemBase* system = nullptr;
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
			std::cerr << "WARNING::SYSTEM_MANAGER::Entity " << entity
			          << " doesn't have all required components for system " << systemType.name() << ". Cant subscribe!"
			          << std::endl;
		}
	}

	template <typename TSystem>
	void Unsubscribe(Entity entity)
	{
		std::type_index systemType = typeid(TSystem);

		auto systemIt = SystemToEntities.find(systemType);
		if (systemIt != SystemToEntities.end())
		{
			auto& entities = systemIt->second;
			auto entityIt = std::find(entities.begin(), entities.end(), entity);
			if (entityIt != entities.end())
			{
				entities.erase(entityIt);
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
		auto entityIt = EntityToSystems.find(entity);
		if (entityIt == EntityToSystems.end()) return;

		auto& entitySystems = entityIt->second;

		for (auto it = entitySystems.begin(); it != entitySystems.end();)
		{
			std::type_index systemType = *it;

			bool shouldRemove = true;
			for (auto& system : Systems)
			{
				if (std::type_index(typeid(*system)) == systemType)
				{
					shouldRemove = !system->ShouldProcessEntity(entity, cm);
					break;
				}
			}

			if (shouldRemove)
			{
				auto& entities = SystemToEntities[systemType];
				entities.erase(std::remove(entities.begin(), entities.end(), entity), entities.end());

				it = entitySystems.erase(it);

				std::cerr << "WARNING::SYSTEM_MANAGER::Entity " << entity << " unsubscribed from system "
				          << systemType.name() << " because it doesn't have all required components" << std::endl;
			}
			else
			{
				++it;
			}
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