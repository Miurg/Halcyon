#pragma once
#include <algorithm>
#include <iostream>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "ISystemContextual.hpp"
#include "ISystemSubscribed.hpp"
#include "../Entitys/ActiveEntitySet.hpp"

class GeneralManager;

enum class SystemKind : uint8_t
{
	Subscribed,
	Contextual
};

struct SystemOrderEntry
{
	SystemKind kind;
	uint32_t index;
};

class SystemManager
{
private:
	std::vector<std::unique_ptr<ISystemContextual>> SystemContextual;
	std::vector<std::unique_ptr<ISystemSubscribed>> SystemsSubscribed;
	std::unordered_map<std::type_index, std::vector<Entity>> SystemToEntities;
	std::unordered_map<Entity, std::vector<std::type_index>> EntityToSystems;
	std::vector<SystemOrderEntry> executionOrder;

public:
	void onShutdown(GeneralManager& gm)
	{
		for (auto it = SystemContextual.rbegin(); it != SystemContextual.rend(); ++it)
		{
			(*it)->onShutdown(gm);
		}
		for (auto it = SystemsSubscribed.rbegin(); it != SystemsSubscribed.rend(); ++it)
		{
			(*it)->onShutdown(gm);
		}
	}

	template <typename TSystem, typename... Args>
	void addSystem(GeneralManager& gm, Args&&... args)
	{
		static_assert(std::is_base_of_v<ISystemSubscribed, TSystem> || std::is_base_of_v<ISystemContextual, TSystem>,
		              "TSystem must derive from either ISystemSubscribed or ISystemContextual");
		if constexpr (std::is_base_of_v<ISystemSubscribed, TSystem>)
		{
			auto system = std::make_unique<TSystem>(std::forward<Args>(args)...);
			SystemsSubscribed.push_back(std::move(system));
			SystemsSubscribed.back()->onRegistered(gm);
			uint32_t index = static_cast<uint32_t>(SystemsSubscribed.size() - 1);
			executionOrder.push_back(SystemOrderEntry{SystemKind::Subscribed, index});
		}
		else
		{
			auto system = std::make_unique<TSystem>(std::forward<Args>(args)...);
			SystemContextual.push_back(std::move(system));
			SystemContextual.back()->onRegistered(gm);
			uint32_t index = static_cast<uint32_t>(SystemContextual.size() - 1);
			executionOrder.push_back(SystemOrderEntry{SystemKind::Contextual, index});
		}
	}

	template <typename TSystem>
	void subscribe(Entity entity, GeneralManager& gm)
	{
		std::type_index systemType = typeid(TSystem);
		ISystemSubscribed* system = nullptr;

		{
			for (auto& sys : SystemsSubscribed)
			{
				if (std::type_index(typeid(*sys)) == systemType)
				{
					system = sys.get();
					break;
				}
			}
		}

		if (!system)
		{
			std::cerr << "WARNING::SYSTEM_MANAGER::Entity " << entity << " trying to subscribe to a non-existent system "
			          << systemType.name() << ". Cant subscribe!" << std::endl;
		}
		else if (!system->shouldProcessEntity(entity, gm))
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
	void unsubscribe(Entity entity)
	{
		std::type_index systemType = typeid(TSystem);

		ISystemSubscribed* system = nullptr;

		{
			for (auto& sys : SystemsSubscribed)
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
			}
		}

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

	void unsubscribeFromAll(Entity entity)
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

	void checkEntitySubscriptions(Entity entity, GeneralManager& gm)
	{
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

		for (auto& system : SystemsSubscribed)
		{
			// Если система не обязательная - скипаем
			if (!system->isSubscribtionMandatory()) continue;

			std::type_index systemType = typeid(*system);

			bool alreadySubscribed = false;
			for (const auto& existingType : currentSystems)
			{
				if (existingType == systemType)
				{
					alreadySubscribed = true;
					break;
				}
			}

			if (alreadySubscribed) continue;

			if (system->shouldProcessEntity(entity, gm))
			{
				SystemToEntities[systemType].push_back(entity);
				EntityToSystems[entity].push_back(systemType);
			}
		}
	}

	void updateSystems(float deltaTime, GeneralManager& gm)
	{
		for (auto& entry : executionOrder)
		{
			if (entry.kind == SystemKind::Subscribed)
			{
				auto& system = SystemsSubscribed[entry.index];
				std::type_index systemType = typeid(*system);
				auto it = SystemToEntities.find(systemType);
				if (it != SystemToEntities.end())
				{
					SystemsSubscribed[entry.index]->update(deltaTime, gm, it->second);
				}
			}
			else
			{
				SystemContextual[entry.index]->update(deltaTime, gm);
			}
		}
	}
};
