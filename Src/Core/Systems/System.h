#pragma once
#include <vector>

#include "../Components/ComponentManager.h"

class SystemBase
{
public:
	virtual ~SystemBase() = default;
	virtual void Update(float deltaTime, ComponentManager& cm, const std::vector<Entity>& entities) = 0;
	virtual bool ShouldProcessEntity(Entity entity, ComponentManager& cm) = 0;
};

template <typename Derived, typename... RequiredComponents>
class System : public SystemBase
{
public:
	virtual ~System() = default;

	virtual void Update(float deltaTime, ComponentManager& cm, const std::vector<Entity>& entities)
	{
		for (Entity entity : entities)
		{
			ProcessEntity(entity, cm, deltaTime);
		}
	}

	virtual inline void ProcessEntity(Entity entity, ComponentManager& cm, float deltaTime) = 0;

	virtual bool ShouldProcessEntity(Entity entity, ComponentManager& cm)
	{
		return HasAllComponents<RequiredComponents...>(entity, cm, typeid(Derived).name());
	}

protected:
	template <typename Component, typename... Rest>
	static bool HasAllComponents(Entity entity, ComponentManager& cm, const char* systemName)
	{
		if (cm.GetComponent<Component>(entity) == nullptr)
		{
			std::cerr << "WARNING::SYSTEM::Entity " << entity << " should not be processed by " << systemName
			          << " because it doesn't have required component: " << typeid(Component).name() << std::endl;
			return false;
		}

		if constexpr (sizeof...(Rest) > 0)
			return HasAllComponents<Rest...>(entity, cm, systemName);
		else
			return true;
	}
};