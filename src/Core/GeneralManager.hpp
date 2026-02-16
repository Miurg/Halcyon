#pragma once

#include <memory>
#include <iostream>
#include <utility>

#include "Components/ComponentManager.hpp"
#include "Entitys/EntityManager.hpp"
#include "Entitys/ActiveEntitySet.hpp"
#include "Systems/SystemManager.hpp"
#include "Contexts/ContextManager.hpp"

// Central ECS manager — coordinates entities, components, systems and contexts.
class GeneralManager
{
private:
	ComponentManager _componentManager;
	EntityManager _entityManager;
	SystemManager _systemManager;
	ContextManager _contextManager;
	ActiveEntitySet _activeEntities;

public:
	GeneralManager(const GeneralManager&) = delete;
	GeneralManager& operator=(const GeneralManager&) = delete;
	GeneralManager(GeneralManager&&) noexcept = default;
	GeneralManager& operator=(GeneralManager&&) noexcept = default;
	GeneralManager() = default;
	// Shuts down systems in reverse registration order before managers are destroyed.
	~GeneralManager()
	{
		_systemManager.onShutdown(*this);
	}

	// Creates a new entity and marks it active.
	Entity createEntity()
	{
		Entity entity = _entityManager.createEntity();
		_activeEntities.insert(entity);
		return entity;
	}

	// Removes components first, then unsubscribes from all systems, then deactivates.
	// Order matters: systems may access components in onEntityUnsubscribed.
	void destroyEntity(Entity entity)
	{
		if (!_activeEntities.contains(entity))
		{
			std::cerr << "WARNING::GENERAL_MANAGER::DestroyEntity on inactive entity " << entity << std::endl;
			return;
		}

		_componentManager.removeEntity(entity);
		_systemManager.unsubscribeFromAll(entity, *this);
		_activeEntities.erase(entity);
	}

	// Adds a component to the entity. Returns pointer to it or nullptr if inactive.
	template <typename TComponent, typename... Args>
	TComponent* addComponent(Entity entity, Args&&... args)
	{
		if (!_activeEntities.contains(entity))
		{
			std::cerr << "WARNING::GENERAL_MANAGER::AddComponent on inactive entity " << entity << std::endl;
			return nullptr;
		}

		TComponent* component = _componentManager.addComponent<TComponent>(entity, std::forward<Args>(args)...);

		return component;
	}

	// Removes a component and auto-unsubscribes from systems whose requirements no longer match.
	template <typename TComponent>
	void removeComponent(Entity entity)
	{
		if (!_activeEntities.contains(entity))
		{
			std::cerr << "WARNING::GENERAL_MANAGER::RemoveComponent on inactive entity " << entity << std::endl;
			return;
		}

		_componentManager.removeComponent<TComponent>(entity);
		_systemManager.checkEntitySubscriptions(entity, *this);
	}

	// Returns pointer to the component or nullptr.
	template <typename TComponent>
	TComponent* getComponent(Entity entity)
	{
		if (!_activeEntities.contains(entity))
		{
			std::cerr << "WARNING::GENERAL_MANAGER::GetComponent on inactive entity " << entity << std::endl;
			return nullptr;
		}

		return _componentManager.getComponent<TComponent>(entity);
	}

	// Registers a new system.
	template <typename TSystem, typename... Args>
	void registerSystem(Args&&... args)
	{
		_systemManager.addSystem<TSystem>(*this, std::forward<Args>(args)...);
	}

	// Subscribes an entity to a system.
	template <typename TSystem>
	void subscribeEntity(Entity entity)
	{
		if (!_activeEntities.contains(entity))
		{
			std::cerr << "WARNING::GENERAL_MANAGER::SubscribeEntity on inactive entity " << entity << std::endl;
			return;
		}

		_systemManager.subscribe<TSystem>(entity, *this);
	}

	// Unsubscribes an entity from a system.
	template <typename TSystem>
	void unsubscribeEntity(Entity entity)
	{
		if (!_activeEntities.contains(entity))
		{
			std::cerr << "WARNING::GENERAL_MANAGER::UnsubscribeEntity on inactive entity " << entity << std::endl;
			return;
		}

		_systemManager.unsubscribe<TSystem>(entity, *this);
	}

	// Updates all systems with the given delta time.
	void update(float deltaTime)
	{
		_systemManager.updateSystems(deltaTime, *this);
	}

	const ActiveEntitySet& getActiveEntities() const noexcept
	{
		return _activeEntities;
	}

	bool isActive(Entity entity) const noexcept
	{
		return _activeEntities.contains(entity);
	}

	// Registers a context — a named entity for global resource access.
	template <typename TContext>
	void registerContext(Entity ctx)
	{
		_contextManager.registerContext<TContext>(ctx);
	}

	template <typename TContext>
	Entity getContext()
	{
		return _contextManager.getContext<TContext>();
	}

	// Gets a component from a context entity. Skips the active-entity check.
	template <typename TContext, typename TComponent>
	TComponent* getContextComponent()
	{
		return _componentManager.getComponent<TComponent>(_contextManager.getContext<TContext>());
	}
};