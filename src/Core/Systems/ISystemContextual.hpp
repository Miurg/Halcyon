#pragma once

class GeneralManager;
class ISystemContextual
{
public:
	virtual ~ISystemContextual() = default;
	virtual void update(float deltaTime, GeneralManager& gm) = 0;
	virtual void onRegistered(GeneralManager& gm) = 0;
	virtual void onShutdown(GeneralManager& gm) = 0;
};