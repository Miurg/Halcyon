#pragma once
#include <vector>
#include "../Entitys/EntityManager.hpp"
#include "../GeneralManager.hpp"
#include <iostream>
#include "ISystemSubscribed.hpp"

template <typename Derived, typename... RequiredComponents>
class SystemSubscribed : public ISystemSubscribed
{
protected:
	template <typename TComponent, typename... Rest>
	static bool hasAllComponents(Entity entity, GeneralManager& gm, const char* systemName)
	{
		if (gm.getComponent<TComponent>(entity) == nullptr)
		{
			std::cerr << "WARNING::SYSTEM::Entity " << entity << " should not be processed by " << systemName
			          << " because it doesn't have required component: " << typeid(TComponent).name() << std::endl;
			return false;
		}

		if constexpr (sizeof...(Rest) > 0)
			return hasAllComponents<Rest...>(entity, gm, systemName);
		else
			return true;
	}

public:
	virtual ~SystemSubscribed() = default;

	virtual void update(float deltaTime, GeneralManager& gm, const std::vector<Entity>& entities)
	{
		for (Entity entity : entities)
		{
			processEntity(entity, gm, deltaTime);
		}
	}

	virtual inline void processEntity(Entity entity, GeneralManager& gm, float deltaTime) = 0;

	virtual bool shouldProcessEntity(Entity entity, GeneralManager& gm)
	{
		return hasAllComponents<RequiredComponents...>(entity, gm, typeid(Derived).name());
	}


	virtual void onRegistered(GeneralManager& gm) = 0;
	virtual void onShutdown(GeneralManager& gm) = 0;
};