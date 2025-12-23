#pragma once

class GeneralManager;
class ISystemContextual
{
public:
	virtual ~ISystemContextual() = default;
	virtual void update(float deltaTime, GeneralManager& gm) = 0;
};