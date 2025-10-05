#pragma once
#include <algorithm>
#include <iostream>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "SystemContextual.h"
#include "SystemSubscribed.h"
class GeneralManager;

class SystemManager
{
private:
	std::vector<std::unique_ptr<ISystemContextual>> SystemContextual;
	std::vector<std::unique_ptr<ISystemSubscribed>> SystemsSubscribed;
	std::unordered_map<std::type_index, std::vector<Entity>> SystemToEntities;
	std::unordered_map<Entity, std::vector<std::type_index>> EntityToSystems;
	mutable std::shared_mutex _mutex;

public:
	template <typename TSystem, typename... Args>
	void AddSystem(Args&&... args)
	{
		std::unique_lock<std::shared_mutex> lock(_mutex);
		static_assert(std::is_base_of_v<ISystemSubscribed, TSystem> || std::is_base_of_v<ISystemContextual, TSystem>,
		              "TSystem must derive from either ISystemSubscribed or ISystemContextual");
		if constexpr (std::is_base_of_v<ISystemSubscribed, TSystem>)
			SystemsSubscribed.emplace_back(std::make_unique<TSystem>(std::forward<Args>(args)...));
		else
			SystemContextual.emplace_back(std::make_unique<TSystem>(std::forward<Args>(args)...));
	}

	template <typename TSystem>
	void Subscribe(Entity entity, GeneralManager& gm)
	{
		std::type_index systemType = typeid(TSystem);
		ISystemSubscribed* system = nullptr;

		{
			std::shared_lock<std::shared_mutex> lock(_mutex);
			for (auto& sys : SystemsSubscribed)
			{
				if (std::type_index(typeid(*sys)) == systemType)
				{
					system = sys.get();
					break;
				}
			}
		}

		std::unique_lock<std::shared_mutex> lock(_mutex);
		if (!system)
		{
			std::cerr << "WARNING::SYSTEM_MANAGER::Entity " << entity << " trying to subscribe to a non-existent system "
			          << systemType.name() << ". Cant subscribe!" << std::endl;
		}
		else if (!system->ShouldProcessEntity(entity, gm))
		{
			std::cerr << "WARNING::SYSTEM_MANAGER::Entity " << entity
			          << " doesn't have all required components for system " << systemType.name() << ". Cant subscribe!"
			          << std::endl;
		}
		else
		{
			SystemToEntities[systemType].push_back(entity);
			EntityToSystems[entity].push_back(systemType);
		}
	}

	template <typename TSystem>
	void Unsubscribe(Entity entity)
	{
		std::type_index systemType = typeid(TSystem);

		ISystemSubscribed* system = nullptr;

		{
			std::shared_lock<std::shared_mutex> lock(_mutex);
			for (auto& sys : SystemsSubscribed)
			{
				if (std::type_index(typeid(*sys)) == systemType)
				{
					system = sys.get();
					break;
				}
			}
			if (system)
			{
				std::cerr << "WARNING::SYSTEM_MANAGER::Entity " << entity
				          << " trying to unsubscribe from a non-existent system " << systemType.name()
				          << ". Cant unsubscribe!" << std::endl;
			}
		}

		std::unique_lock<std::shared_mutex> lock(_mutex);
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
		std::unique_lock<std::shared_mutex> lock(_mutex);
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

	void CheckEntitySubscriptions(Entity entity, GeneralManager& gm)
	{
		std::shared_lock<std::shared_mutex> lock(_mutex);
		auto entityIt = EntityToSystems.find(entity);
		if (entityIt == EntityToSystems.end()) return;

		auto& entitySystems = entityIt->second;

		for (auto it = entitySystems.begin(); it != entitySystems.end();)
		{
			std::type_index systemType = *it;

			bool shouldRemove = true;
			for (auto& system : SystemsSubscribed)
			{
				if (std::type_index(typeid(*system)) == systemType)
				{
					shouldRemove = !system->ShouldProcessEntity(entity, gm);
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

	void UpdateSystems(float deltaTime, GeneralManager& gm)
	{
		for (auto& system : SystemsSubscribed)
		{
			std::type_index systemType = typeid(*system);
			auto it = SystemToEntities.find(systemType);
			if (it != SystemToEntities.end())
			{
				system->Update(deltaTime, gm, it->second);
			}
		}
		for (auto& system : SystemContextual)
		{
			system->Update(deltaTime, gm);
		}
	}
};