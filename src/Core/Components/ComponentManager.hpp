#pragma once
#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "../Entitys/EntityManager.hpp"
#include "ComponentArray.hpp"

class ComponentManager
{
private:
	template <typename TComponent>
	static ComponentArray<TComponent>& getComponentArray()
	{
		static ComponentArray<TComponent> array;
		return array;
	}

	static std::unordered_map<std::type_index, std::function<void(Entity)>> _removeCallbacks;

public:
	template <typename TComponent, typename... Args>
	TComponent* addComponent(Entity entity, Args&&... args)
	{
		return getComponentArray<TComponent>().addComponent(entity, TComponent{std::forward<Args>(args)...});
	}

	template <typename TComponent>
	TComponent* getComponent(Entity entity)
	{
		return getComponentArray<TComponent>().getComponent(entity);
	}

	template <typename TComponent>
	void removeComponent(Entity entity)
	{
		getComponentArray<TComponent>().removeComponent(entity);
	}

	template <typename TComponent>
	ComponentArray<TComponent>& getAllComponents()
	{
		return getComponentArray<TComponent>();
	}

	void removeEntity(Entity entity)
	{
		for (auto& [type, callback] : _removeCallbacks)
		{
			callback(entity);
		}
	}

	template <typename TComponent>
	static void registerComponentType()
	{
		_removeCallbacks[std::type_index(typeid(TComponent))] = [](Entity entity)
		{ getComponentArray<TComponent>().removeComponent(entity); };
	}
};
