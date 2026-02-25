#pragma once
#include <vector>
#include "../Entitys/EntityManager.hpp"
#include "ISystemCore.hpp"
#include "../GeneralManager.hpp"
#include <iostream>

// CRTP base for systems. RequiredComponents are auto-checked in shouldProcessEntity().
template <typename Derived, typename... RequiredComponents>
class SystemCore : public ISystemCore
{
protected:
	template <typename... Comps>
	static bool hasAllComponents(Entity entity, GeneralManager& gm, const char* systemName)
	{
		return (checkComponent<Comps>(entity, gm, systemName) && ...);
	}
	template <typename T>
	static bool checkComponent(Entity entity, GeneralManager& gm, const char* systemName)
	{
		if (gm.getComponent<T>(entity) == nullptr)
		{
			std::cerr << "WARNING::SYSTEM::Entity " << entity << " should not be processed by " << systemName
			          << " because it doesn't have required component: " << typeid(T).name() << std::endl;
			return false;
		}
		return true;
	}

public:
	virtual ~SystemCore() = default;

	virtual void update(GeneralManager& gm)
	{
		// Optional: Override in derived class if needed
	}

	virtual bool shouldProcessEntity(Entity entity, GeneralManager& gm)
	{
		return hasAllComponents<RequiredComponents...>(entity, gm, typeid(Derived).name());
	}

	virtual std::vector<std::type_index> getSystemDependencies() const
	{
		return {};
	}

	virtual void onRegistered(GeneralManager& gm)
	{
		// Optional: Override in derived class if needed
	}

	virtual void onShutdown(GeneralManager& gm)
	{
		// Optional: Override in derived class if needed
	}

	virtual void onEntitySubscribed(Entity entity, GeneralManager& gm)
	{
		if (!shouldProcessEntity(entity, gm))
		{
			return;
		}
	}

	virtual void onEntityUnsubscribed(Entity entity, GeneralManager& gm)
	{
		// Optional: Override in derived class if needed
	}
};