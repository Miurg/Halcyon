#pragma once
#include <vector>

#include "../Entitys/EntityManager.h"
class GeneralManager;
class ISystemContextual
{
public:
	virtual ~ISystemContextual() = default;
	virtual void Update(float deltaTime, GeneralManager& gm) = 0;
};

template <typename Derived>
class SystemContextual : public ISystemContextual
{
public:
	virtual ~SystemContextual() = default;
};