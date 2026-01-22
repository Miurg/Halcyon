#pragma once
#include <vector>

#include "../Entitys/EntityManager.hpp"
#include "ISystemContextual.hpp"
#include "../GeneralManager.hpp"

template <typename Derived>
class SystemContextual : public ISystemContextual
{
public:
	virtual ~SystemContextual() = default;
};