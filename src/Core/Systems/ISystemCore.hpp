#pragma once

#include <vector>
#include <typeindex>
class GeneralManager;
// System interface. Lifecycle: onRegistered → (onEntitySubscribed/update/onEntityUnsubscribed)* → onShutdown.
class ISystemCore
{
public:
	virtual ~ISystemCore() = default;
	virtual void update(GeneralManager& gm) = 0;
	virtual bool shouldProcessEntity(Entity entity, GeneralManager& gm) = 0;
	virtual void onRegistered(GeneralManager& gm) = 0;
	virtual void onShutdown(GeneralManager& gm) = 0;
	virtual std::vector<std::type_index> getSystemDependencies() const = 0;
	virtual void onEntitySubscribed(Entity entity, GeneralManager& gm) = 0;
	virtual void onEntityUnsubscribed(Entity entity, GeneralManager& gm) = 0;
};