#pragma once

#include <unordered_set>

#include "Components/ComponentManager.h"
#include "Entitys/EntityManager.h"
#include "Systems/System.h"
#include "Systems/SystemManager.h"

class GeneralManager
{
private:
	ComponentManager _componentManager;
	EntityManager _entityManager;
	SystemManager _systemManager;
	ContextManager _contextManager;
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

	template <typename TComponent>
	void RegisterComponentType()
	{
		_componentManager.RegisterComponentType<TComponent>();
	}

	template <typename TComponent, typename... Args>
	TComponent* AddComponent(Entity entity, Args&&... args)
	{
		TComponent* component = _componentManager.AddComponent<TComponent>(entity, std::forward<Args>(args)...);
		return component;
	}

	template <typename TComponent>
	void RemoveComponent(Entity entity)
	{
		_componentManager.RemoveComponent<TComponent>(entity);
		_systemManager.CheckEntitySubscriptions(entity, _componentManager);
	}

	template <typename TComponent>
	TComponent* GetComponent(Entity entity)
	{
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
		_systemManager.Subscribe<TSystem>(entity, _componentManager);
	}

	template <typename TSystem>
	void UnsubscribeEntity(Entity entity)
	{
		_systemManager.Unsubscribe<TSystem>(entity);
	}

	void Update(float deltaTime)
	{
		_systemManager.UpdateSystems(deltaTime, _componentManager, _contextManager);
	}

	const std::unordered_set<Entity>& GetActiveEntities() const
	{
		return _activeEntities;
	}

	template <typename TContext>
	void RegisterContext(std::shared_ptr<TContext> ctx)
	{
		_contextManager.RegisterContext<TContext>(ctx);
	}

	template <typename TContext>
	std::shared_ptr<TContext> GetContext()
	{
		_contextManager.GetContext<TContext>();
	}
};