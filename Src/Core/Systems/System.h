#pragma once
#include <vector>

#include "../Components/ComponentManager.h"
#include "../Contexts/ContextManager.h"

class SystemBase
{
public:
	virtual ~SystemBase() = default;
	virtual void Update(float deltaTime, ComponentManager& cm,
	                    ContextManager& ctxM, const std::vector<Entity>& entities) = 0;
	virtual bool ShouldProcessEntity(Entity entity, ComponentManager& cm) = 0;
};

template <typename Derived, typename... RequiredComponents>
class System : public SystemBase
{
protected:
	template <typename TComponent, typename... Rest>
	static bool HasAllComponents(Entity entity, ComponentManager& cm, const char* systemName)
	{
		if (cm.GetComponent<TComponent>(entity) == nullptr)
		{
			std::cerr << "WARNING::SYSTEM::Entity " << entity << " should not be processed by " << systemName
			          << " because it doesn't have required component: " << typeid(TComponent).name() << std::endl;
			return false;
		}

		if constexpr (sizeof...(Rest) > 0)
			return HasAllComponents<Rest...>(entity, cm, systemName);
		else
			return true;
	}
public:
	virtual ~System() = default;

	virtual void Update(float deltaTime, ComponentManager& cm, ContextManager& ctxM,
	                    const std::vector<Entity>& entities)
	{
		for (Entity entity : entities)
		{
			ProcessEntity(entity, cm, ctxM, deltaTime);
		}
	}

	virtual inline void ProcessEntity(Entity entity, ComponentManager& cm, ContextManager& ctxM, float deltaTime) = 0;

	virtual bool ShouldProcessEntity(Entity entity, ComponentManager& cm)
	{
		return HasAllComponents<RequiredComponents...>(entity, cm, typeid(Derived).name());
	}
};