#pragma once

#include <vector>

class GeneralManager;
class ISystemSubscribed
{
public:
	virtual ~ISystemSubscribed() = default;
	virtual void update(float deltaTime, GeneralManager& gm, const std::vector<Entity>& entities) = 0;
	virtual bool shouldProcessEntity(Entity entity, GeneralManager& gm) = 0;
	virtual void onRegistered(GeneralManager& gm) = 0;
	virtual void onShutdown(GeneralManager& gm) = 0;
};