#pragma once

#include <memory>
#include <iostream>
#include <utility>

#include "Components/ComponentManager.hpp"
#include "Entitys/EntityManager.hpp"
#include "Entitys/ActiveEntitySet.hpp"
#include "Systems/SystemManager.hpp"
#include "Contexts/ContextManager.hpp"

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
	~GeneralManager() 
	{
		_systemManager.onShutdown(*this);
	}

	// Create a new entity and mark it as active
	Entity createEntity()
	{
		Entity entity = _entityManager.createEntity();
		_activeEntities.insert(entity);
		return entity;
	}

	// Destroy an entity: remove components, unsubscribe from systems and mark inactive
	void destroyEntity(Entity entity)
	{
		if (!_activeEntities.contains(entity))
		{
			std::cerr << "WARNING::GENERAL_MANAGER::DestroyEntity on inactive entity " << entity << std::endl;
			return;
		}

		_componentManager.removeEntity(entity);
		_systemManager.unsubscribeFromAll(entity);
		_activeEntities.erase(entity);
	}


	template <typename TComponent, typename... Args>
	TComponent* addComponent(Entity entity, Args&&... args)
	{
		if (!_activeEntities.contains(entity))
		{
			std::cerr << "WARNING::GENERAL_MANAGER::AddComponent on inactive entity " << entity << std::endl;
			return nullptr;
		}

		TComponent* component = _componentManager.addComponent<TComponent>(entity, std::forward<Args>(args)...);
		
		_systemManager.checkForNewSubscriptions(entity, *this);

		return component;
	}

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

	template <typename TSystem, typename... Args>
	void registerSystem(Args&&... args)
	{
		_systemManager.addSystem<TSystem>(*this, std::forward<Args>(args)...);
	}

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

	template <typename TSystem>
	void unsubscribeEntity(Entity entity)
	{
		if (!_activeEntities.contains(entity))
		{
			std::cerr << "WARNING::GENERAL_MANAGER::UnsubscribeEntity on inactive entity " << entity << std::endl;
			return;
		}

		_systemManager.unsubscribe<TSystem>(entity);
	}

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

	template <typename TContext, typename TComponent>
	TComponent* getContextComponent()
	{
		return _componentManager.getComponent<TComponent>(_contextManager.getContext<TContext>());
	}
};