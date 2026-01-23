#pragma once
#include <algorithm>
#include <iostream>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "ISystemCore.hpp"
#include "../Entitys/ActiveEntitySet.hpp"

class GeneralManager;

class SystemManager
{
private:
	std::vector<std::unique_ptr<ISystemCore>> SystemCore;
	std::unordered_map<std::type_index, std::vector<Entity>> SystemToEntities;
	std::unordered_map<Entity, std::vector<std::type_index>> EntityToSystems;

	void subscribeInternal(Entity entity, std::type_index systemType, GeneralManager& gm)
	{
		ISystemCore* system = nullptr;

		for (auto& sys : SystemCore)
		{
			if (std::type_index(typeid(*sys)) == systemType)
			{
				system = sys.get();
				break;
			}
		}

		if (!system)
		{
			std::cerr << "WARNING::SYSTEM_MANAGER::Entity " << entity << " trying to subscribe to a non-existent system "
			          << systemType.name() << ". Cant subscribe!" << std::endl;
			return;
		}

		if (!system->shouldProcessEntity(entity, gm))
		{
			std::cerr << "WARNING::SYSTEM_MANAGER::Entity " << entity
			          << " doesn't have all required components for system " << systemType.name() << ". Cant subscribe!"
			          << std::endl;
			return;
		}

		auto& currentSystems = EntityToSystems[entity];

		if (std::find(currentSystems.begin(), currentSystems.end(), systemType) != currentSystems.end())
		{
			std::cout << "WARNING::SYSTEM_MANAGER::Entity " << entity
			          << " trying to subscribe to a system that is already subscribed to  " << systemType.name()
			          << ". Cant subscribe!" << std::endl;
			return;
		}

		system->onEntitySubscribed(entity, gm);

		SystemToEntities[systemType].push_back(entity);
		EntityToSystems[entity].push_back(systemType);

		const auto& dependencies = system->getSystemDependencies();
		for (const auto& depType : dependencies)
		{
			subscribeInternal(entity, depType, gm);
		}
	}

public:
	void onShutdown(GeneralManager& gm)
	{
		for (auto it = SystemCore.rbegin(); it != SystemCore.rend(); ++it)
		{
			(*it)->onShutdown(gm);
		}
	}

	template <typename TSystem, typename... Args>
	void addSystem(GeneralManager& gm, Args&&... args)
	{
		static_assert(std::is_base_of_v<ISystemCore, TSystem>, "TSystem must derive from ISystemCore");
		auto system = std::make_unique<TSystem>(std::forward<Args>(args)...);
		SystemCore.push_back(std::move(system));
		SystemCore.back()->onRegistered(gm);
	}

	template <typename TSystem>
	void subscribe(Entity entity, GeneralManager& gm)
	{
		subscribeInternal(entity, typeid(TSystem), gm);
	}

	void unsubscribe(Entity entity, std::type_index systemType, GeneralManager& gm)
	{
		ISystemCore* system = nullptr;

		{
			for (auto& sys : SystemCore)
			{
				if (std::type_index(typeid(*sys)) == systemType)
				{
					system = sys.get();
					break;
				}
			}
			if (!system)
			{
				std::cerr << "WARNING::SYSTEM_MANAGER::Entity " << entity
				          << " trying to unsubscribe from a non-existent system " << systemType.name()
				          << ". Cant unsubscribe!" << std::endl;
				return;
			}
		}

		system->onEntityUnsubscribed(entity, gm);
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

	template <typename TSystem>
	void unsubscribe(Entity entity, GeneralManager& gm)
	{
		unsubscribe(entity, std::type_index(typeid(TSystem)), gm);
	}

	void unsubscribeFromAll(Entity entity, GeneralManager& gm)
	{
		auto it = EntityToSystems.find(entity);
		if (it != EntityToSystems.end())
		{
			std::vector<std::type_index> systems = it->second;
			for (const auto& systemType : systems)
			{
				unsubscribe(entity, systemType, gm);
			}
		}
	}

	void checkEntitySubscriptions(Entity entity, GeneralManager& gm)
	{
		auto entityIt = EntityToSystems.find(entity);
		if (entityIt == EntityToSystems.end()) return;

		auto& entitySystems = entityIt->second;

		for (auto it = entitySystems.begin(); it != entitySystems.end();)
		{
			std::type_index systemType = *it;

			bool shouldRemove = true;
			for (auto& system : SystemCore)
			{
				if (std::type_index(typeid(*system)) == systemType)
				{
					shouldRemove = !system->shouldProcessEntity(entity, gm);
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

	void checkForNewSubscriptions(Entity entity, GeneralManager& gm)
	{
		auto& currentSystems = EntityToSystems[entity];

		for (auto& system : SystemCore)
		{
			if (!system->isSubscribtionMandatory()) continue;

			std::type_index systemType = typeid(*system);

			subscribeInternal(entity, systemType, gm);
		}
	}

	void updateSystems(float deltaTime, GeneralManager& gm)
	{
		for (auto& entry : SystemCore)
		{
			entry->update(deltaTime, gm);
		}
	}
};
