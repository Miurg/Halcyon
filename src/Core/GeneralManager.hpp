#pragma once

#include <memory>
#include <iostream>
#include <utility>

#include "Components/ComponentManager.hpp"
#include "Entitys/EntityManager.hpp"
#include "Entitys/ActiveEntitySet.hpp"
#include "Systems/SystemManager.hpp"
#include "Contexts/ContextManager.hpp"

/// <summary>
/// Central manager class that coordinates all ECS (Entity-Component-System) functionality.
/// This class serves as the main interface for entity lifecycle management, component operations,
/// system registration and updates, and context management. It maintains the core ECS architecture
/// by managing active entities and delegating operations to specialized managers.
/// </summary>
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

	/// <summary>
	/// Creates a new Entity using the entity manager and registers it as active.
	/// </summary>
	/// <returns>The newly created Entity (also inserted into the active entities set).</returns>
	Entity createEntity()
	{
		Entity entity = _entityManager.createEntity();
		_activeEntities.insert(entity);
		return entity;
	}

	/// <summary>
	/// Destroys an active entity by removing its components, unsubscribing it from all systems, and erasing it from the
	/// active entity set. If the entity is not active, logs a warning and takes no action.
	/// </summary>
	/// <param name="entity">Identifier or handle of the entity to destroy. If the entity is not present in the active
	/// set, the function logs a warning and returns without performing removals.</param>
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

	/// <summary>
	/// Adds a component of the specified type to the given entity and checks for new system subscriptions.
	/// The component is constructed with the provided arguments using perfect forwarding.
	/// After adding the component, the system manager is notified to check if the entity should be
	/// subscribed to any systems that now match its component signature.
	/// </summary>
	/// <typeparam name="TComponent">The component type to add to the entity.</typeparam>
	/// <typeparam name="...Args">Parameter types for component construction.</typeparam>
	/// <param name="entity">The entity to which the component will be added.</param>
	/// <param name="...args">Arguments to forward to the component constructor.</param>
	/// <returns>Pointer to the newly added component, or nullptr if the entity is not active.</returns>
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

	/// <summary>
	/// Removes a component of the specified type from the given entity if the entity is active; logs a warning for
	/// inactive entities and updates system subscriptions after removal.
	/// </summary>
	/// <typeparam name="TComponent">The component type to remove from the entity.</typeparam>
	/// <param name="entity">The identifier of the entity from which to remove the component.</param>
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

	/// <summary>
	/// Retrieves a pointer to the component of type TComponent associated with the given entity. If the entity is not
	/// active, logs a warning and returns nullptr.
	/// </summary>
	/// <typeparam name="TComponent">The component type to retrieve.</typeparam>
	/// <param name="entity">The entity identifier whose component to retrieve.</param>
	/// <returns>A pointer to the component of type TComponent if the entity is active and the component exists;
	/// otherwise nullptr. If the entity is inactive, a warning is written to std::cerr.</returns>
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

	/// <summary>
	/// Registers a new system with the ECS architecture using the provided constructor arguments.
	/// The system is instantiated and added to the system manager, which will handle its lifecycle
	/// and update calls. Systems should implement the required interface for proper integration.
	/// </summary>
	/// <typeparam name="TSystem">The system type to register (must implement ISystemCore interface).</typeparam>
	/// <typeparam name="...Args">Parameter types for system construction.</typeparam>
	/// <param name="...args">Arguments to forward to the system constructor.</param>
	template <typename TSystem, typename... Args>
	void registerSystem(Args&&... args)
	{
		_systemManager.addSystem<TSystem>(*this, std::forward<Args>(args)...);
	}

	/// <summary>
	/// Subscribes an active entity to the specified system.
	/// </summary>
	/// <typeparam name="TSystem">The system type to which the entity will be subscribed (used by the system
	/// manager).</typeparam> <param name="entity">The entity to subscribe. If the entity is inactive, the function logs
	/// a warning and returns without subscribing.</param>
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

	/// <summary>
	/// Unsubscribes the given entity from the specified system. If the entity is not active, logs a warning and does
	/// nothing.
	/// </summary>
	/// <typeparam name="TSystem">The system type from which the entity should be unsubscribed.</typeparam>
	/// <param name="entity">The entity to unsubscribe. If the entity is not in the active set, the function logs a
	/// warning to std::cerr and returns without unsubscribing.</param>
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

	/// <summary>
	/// Forwards the elapsed time to the system manager to update the object's systems.
	/// </summary>
	/// <param name="deltaTime">Elapsed time since the last update (time step), typically in seconds.</param>
	void update(float deltaTime)
	{
		_systemManager.updateSystems(deltaTime, *this);
	}

	/// <summary>
	/// Returns a const reference to the set of active entities. This accessor is const and noexcept and does not modify
	/// or throw.
	/// </summary>
	/// <returns>A const reference to the ActiveEntitySet (_activeEntities). The reference refers to the object's
	/// internal storage and remains valid as long as the object is not destroyed or the member is not
	/// modified.</returns>
	const ActiveEntitySet& getActiveEntities() const noexcept
	{
		return _activeEntities;
	}

	/// <summary>
	/// Checks whether the specified entity is active.
	/// </summary>
	/// <param name="entity">The entity to check for activity.</param>
	/// <returns>true if the entity is active (present in _activeEntities); otherwise false.</returns>
	bool isActive(Entity entity) const noexcept
	{
		return _activeEntities.contains(entity);
	}

	/// <summary>
	/// Registers a context of the specified type for the given entity using the internal context manager.
	/// Contexts are special entities that provide global access to shared resources and are identified by type.
	/// </summary>
	/// <typeparam name="TContext">The context type identifier used for retrieval.</typeparam>
	/// <param name="ctx">The entity that serves as the context container for this context type.</param>
	template <typename TContext>
	void registerContext(Entity ctx)
	{
		_contextManager.registerContext<TContext>(ctx);
	}

	/// <summary>
	/// Retrieves the Entity associated with the specified context type from the context manager.
	/// </summary>
	/// <typeparam name="TContext">The context type to retrieve; selects which context Entity is returned.</typeparam>
	/// <returns>An Entity corresponding to the requested context type (returned by value).</returns>
	template <typename TContext>
	Entity getContext()
	{
		return _contextManager.getContext<TContext>();
	}

	/// <summary>
	/// Retrieves a component of type TComponent associated with the context entity of type TContext.
	/// This is a convenience method that combines context lookup with component retrieval,
	/// useful for accessing global components stored in context entities.
	/// </summary>
	/// <typeparam name="TContext">The context type identifier to look up the context entity.</typeparam>
	/// <typeparam name="TComponent">The component type to retrieve from the context entity.</typeparam>
	/// <returns>Pointer to the TComponent instance if found, or nullptr if the context doesn't exist
	/// or the component is not attached to the context entity.</returns>
	template <typename TContext, typename TComponent>
	TComponent* getContextComponent()
	{
		return _componentManager.getComponent<TComponent>(_contextManager.getContext<TContext>());
	}
};