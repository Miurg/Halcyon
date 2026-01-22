#pragma once

#include <vector>
#include <typeindex>

class GeneralManager;
class ISystemSubscribed
{
public:
	virtual ~ISystemSubscribed() = default;
	virtual void update(float deltaTime, GeneralManager& gm, const std::vector<Entity>& entities) = 0;
	virtual inline void processEntity(Entity entity, GeneralManager& gm, float deltaTime) = 0;
	virtual bool shouldProcessEntity(Entity entity, GeneralManager& gm) = 0;
	virtual void onRegistered(GeneralManager& gm) = 0;
	virtual void onShutdown(GeneralManager& gm) = 0;
	virtual bool isSubscribtionMandatory() const = 0;
	virtual std::vector<std::type_index> getSystemDependencies() const = 0;
};