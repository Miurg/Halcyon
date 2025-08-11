#pragma once
#include <cstdint>

using Entity = uint32_t;

class EntityManager
{
private:
	Entity _nextEntity;

public:
	EntityManager() : _nextEntity(1) {}
	Entity CreateEntity()
	{
		return _nextEntity++;
	}
};
