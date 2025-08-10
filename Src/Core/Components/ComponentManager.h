#pragma once
#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "../Entitys/EntityManager.h"
#include "ComponentArray.h"

struct Component
{
	virtual ~Component() = default;
};

class ComponentManager
{
private:
	template <typename TComponent>
	static ComponentArray<TComponent>& GetComponentArray()
	{
		static ComponentArray<TComponent> array;
		return array;
	}

	static std::unordered_map<std::type_index, std::function<void(Entity)>> removeCallbacks;

public:
	template <typename TComponent, typename... Args>
	TComponent* AddComponent(Entity entity, Args&&... args)
	{
		return GetComponentArray<TComponent>().AddComponent(entity, TComponent{std::forward<Args>(args)...});
	}

	template <typename TComponent>
	TComponent* GetComponent(Entity entity)
	{
		return GetComponentArray<TComponent>().GetComponent(entity);
	}

	template <typename TComponent>
	void RemoveComponent(Entity entity)
	{
		GetComponentArray<TComponent>().RemoveComponent(entity);
	}

	template <typename TComponent>
	ComponentArray<TComponent>& GetAllComponents()
	{
		return GetComponentArray<TComponent>();
	}

	void RemoveEntity(Entity entity)
	{
		for (auto& [type, callback] : removeCallbacks)
		{
			callback(entity);
		}
	}

	template <typename TComponent>
	static void RegisterComponentType()
	{
		removeCallbacks[std::type_index(typeid(TComponent))] = [](Entity entity)
		{ GetComponentArray<TComponent>().RemoveComponent(entity); };
	}
};
