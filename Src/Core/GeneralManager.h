#pragma once

#include <memory>
#include <iostream>

#include "Components/ComponentManager.h"
#include "Entitys/EntityManager.h"
#include "Entitys/ActiveEntitySet.h"
#include "Systems/SystemManager.h"
#include "Contexts/ContextManager.h"

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
	~GeneralManager() = default; // kept default; managers are value members

	//Create a new entity and mark it as active
	Entity CreateEntity()
	{
		Entity entity = _entityManager.CreateEntity();
		_activeEntities.insert(entity);
		return entity;
	}

	//Destroy an entity: remove components, unsubscribe from systems and mark inactive
	void DestroyEntity(Entity entity)
	{
		if (!_activeEntities.contains(entity))
		{
			std::cerr << "WARNING::GENERAL_MANAGER::DestroyEntity on inactive entity " << entity << std::endl;
			return;
		}

		_componentManager.RemoveEntity(entity);
		_systemManager.UnsubscribeFromAll(entity);
		_activeEntities.erase(entity);
	}

	template <typename TComponent>
	void RegisterComponentType()
	{
		_componentManager.RegisterComponentType<TComponent>();
	}

	template <typename TComponent, typename... Args>
	TComponent* AddComponent(Entity entity, Args&&... args)
	{
		if (!_activeEntities.contains(entity))
		{
			std::cerr << "WARNING::GENERAL_MANAGER::AddComponent on inactive entity " << entity << std::endl;
			return nullptr;
		}

		TComponent* component = _componentManager.AddComponent<TComponent>(entity, std::forward<Args>(args)...);
		return component;
	}

	template <typename TComponent>
	void RemoveComponent(Entity entity)
	{
		if (!_activeEntities.contains(entity))
		{
			std::cerr << "WARNING::GENERAL_MANAGER::RemoveComponent on inactive entity " << entity << std::endl;
			return;
		}

		_componentManager.RemoveComponent<TComponent>(entity);
		_systemManager.CheckEntitySubscriptions(entity, *this);
	}

	template <typename TComponent>
	TComponent* GetComponent(Entity entity)
	{
		if (!_activeEntities.contains(entity))
		{
			std::cerr << "WARNING::GENERAL_MANAGER::GetComponent on inactive entity " << entity << std::endl;
			return nullptr;
		}

		return _componentManager.GetComponent<TComponent>(entity);
	}

	template <typename TSystem, typename... Args>
	void RegisterSystem(Args&&... args)
	{
		_systemManager.AddSystem<TSystem>(std::forward<Args>(args)...);
	}

	template <typename TSystem>
	void SubscribeEntity(Entity entity)
	{
		if (!_activeEntities.contains(entity))
		{
			std::cerr << "WARNING::GENERAL_MANAGER::SubscribeEntity on inactive entity " << entity << std::endl;
			return;
		}

		_systemManager.Subscribe<TSystem>(entity, *this);
	}

	template <typename TSystem>
	void UnsubscribeEntity(Entity entity)
	{
		if (!_activeEntities.contains(entity))
		{
			std::cerr << "WARNING::GENERAL_MANAGER::UnsubscribeEntity on inactive entity " << entity << std::endl;
			return;
		}

		_systemManager.Unsubscribe<TSystem>(entity);
	}

	void Update(float deltaTime)
	{
		_systemManager.UpdateSystems(deltaTime, *this);
	}

	const ActiveEntitySet& GetActiveEntities() const noexcept
	{
		return _activeEntities;
	}

	bool IsActive(Entity entity) const noexcept
	{
		return _activeEntities.contains(entity);
	}

	template <typename TContext>
	void RegisterContext(std::shared_ptr<TContext> ctx)
	{
		_contextManager.RegisterContext<TContext>(ctx);
	}

	template <typename TContext>
	std::shared_ptr<TContext> GetContext()
	{
		return _contextManager.GetContext<TContext>();
	}
};