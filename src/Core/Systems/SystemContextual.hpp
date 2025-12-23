#pragma once
#include <vector>

#include "../Entitys/EntityManager.hpp"
#include "ISystemConextual.hpp"

template <typename Derived>
class SystemContextual : public ISystemContextual
{
public:
	virtual ~SystemContextual() = default;
};