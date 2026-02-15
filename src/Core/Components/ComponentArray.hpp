#pragma once
#include "StablePool.hpp"
#include <limits>
#include <vector>
template <typename TComponent, uint32_t PoolBlockSize = 4096>
class ComponentArray
{
private:
	StablePool<TComponent, PoolBlockSize> _pool;

	std::vector<uint32_t> _sparse;
	static constexpr uint32_t INVALID_INDEX = std::numeric_limits<uint32_t>::max();

public:
	TComponent* addComponent(Entity entity, TComponent&& component)
	{
		if (entity >= _sparse.size())
		{
			_sparse.resize(entity * 2 + 1, INVALID_INDEX);
		}

		if (_sparse[entity] != INVALID_INDEX)
		{
			uint32_t index = _sparse[entity];
			_pool[index] = std::move(component);
			return _pool.at(index);
		}

		auto [newIndex, ptr] = _pool.allocate(std::move(component));

		_sparse[entity] = newIndex;
		return ptr;
	}

	TComponent* getComponent(Entity entity)
	{
		if (entity >= _sparse.size()) [[unlikely]]
		{
			return nullptr;
		}
		uint32_t index = _sparse[entity];

		if (index == INVALID_INDEX) [[unlikely]]
		{
			return nullptr;
		}

		return _pool.at(index);
	}

	void removeComponent(Entity entity)
	{
		if (entity >= _sparse.size()) [[unlikely]]
			return;
		
		uint32_t indexToRemove = _sparse[entity];

		if (indexToRemove == INVALID_INDEX) [[unlikely]]
			return;

		_pool.deallocate(indexToRemove);

		_sparse[entity] = INVALID_INDEX;
	}

	size_t size() const
	{
		return _pool.liveCount();
	}

	void reserve(size_t capacity)
	{
		uint32_t numBlocks = static_cast<uint32_t>((capacity + PoolBlockSize - 1) / PoolBlockSize);
		_pool.reserveBlocks(numBlocks);
	}
};