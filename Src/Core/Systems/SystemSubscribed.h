#pragma once
#include <vector>
#include "../Entitys/EntityManager.h"
class GeneralManager;
class ISystemSubscribed
{
public:
	virtual ~ISystemSubscribed() = default;
	virtual void Update(float deltaTime, GeneralManager& gm, const std::vector<Entity>& entities) = 0;
	virtual bool ShouldProcessEntity(Entity entity, GeneralManager& gm) = 0;
};

template <typename Derived, typename... RequiredComponents>
class SystemSubscribed : public ISystemSubscribed
{
protected:
	template <typename TComponent, typename... Rest>
	static bool HasAllComponents(Entity entity, GeneralManager& gm, const char* systemName)
	{
		if (gm.GetComponent<TComponent>(entity) == nullptr)
		{
			std::cerr << "WARNING::SYSTEM::Entity " << entity << " should not be processed by " << systemName
			          << " because it doesn't have required component: " << typeid(TComponent).name() << std::endl;
			return false;
		}

		if constexpr (sizeof...(Rest) > 0)
			return HasAllComponents<Rest...>(entity, gm, systemName);
		else
			return true;
	}
public:
	virtual ~SystemSubscribed() = default;

	virtual void Update(float deltaTime, GeneralManager& gm,
	                    const std::vector<Entity>& entities)
	{
		for (Entity entity : entities)
		{
			ProcessEntity(entity, gm, deltaTime);
		}
	}

	virtual inline void ProcessEntity(Entity entity, GeneralManager& gm, float deltaTime) = 0;

	virtual bool ShouldProcessEntity(Entity entity, GeneralManager& gm)
	{
		return HasAllComponents<RequiredComponents...>(entity, gm, typeid(Derived).name());
	}
};