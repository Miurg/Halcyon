#pragma once
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "../Entitys/EntityManager.h"
#include "ComponentArray.h"
#include <functional>

struct Component
{
	virtual ~Component() = default;
};

class ComponentManager
{
private:
	template <typename T>
	static ComponentArray<T>& GetComponentArray()
	{
		static ComponentArray<T> array;
		return array;
	}

	static std::unordered_map<std::type_index, std::function<void(Entity)>> removeCallbacks;

public:
	template <typename T, typename... Args>
	T* AddComponent(Entity entity, Args&&... args)
	{
		return GetComponentArray<T>().AddComponent(entity, T{std::forward<Args>(args)...});
	}

	template <typename T>
	T* GetComponent(Entity entity)
	{
		return GetComponentArray<T>().GetComponent(entity);
	}

	template <typename T>
	void RemoveComponent(Entity entity)
	{
		GetComponentArray<T>().RemoveComponent(entity);
	}

	template <typename T>
	ComponentArray<T>& GetAllComponents()
	{
		return GetComponentArray<T>();
	}

	void RemoveEntity(Entity entity)
	{
		for (auto& [type, callback] : removeCallbacks)
		{
			callback(entity);
		}
	}

	template <typename T>
	static void RegisterComponentType()
	{
		removeCallbacks[std::type_index(typeid(T))] = [](Entity entity)
		{ GetComponentArray<T>().RemoveComponent(entity); };
	}
};
