#pragma once
#include <cstdint>
#include <atomic>

using Entity = uint32_t;

class EntityManager
{
private:
	std::atomic<Entity> _nextEntity;

public:
	EntityManager() : _nextEntity(1) {}

	Entity CreateEntity()
	{
		return _nextEntity.fetch_add(1, std::memory_order_relaxed);
	}
};
