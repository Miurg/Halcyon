#pragma once
#include <limits>
#include <vector>

#include "../Entitys/EntityManager.h"

template <typename TComponent>
class ComponentArray
{
private:
	std::vector<TComponent> _dense;
	std::vector<Entity> _denseToEntity;

	std::vector<uint32_t> _sparse;
	static constexpr uint32_t INVALID_INDEX = std::numeric_limits<uint32_t>::max();

	bool _needsSorting = false;

public:
	TComponent* AddComponent(Entity entity, TComponent&& component)
	{
		if (entity >= _sparse.size())
		{
			_sparse.resize(entity + 1, INVALID_INDEX);
		}

		if (_sparse[entity] != INVALID_INDEX)
		{
			_dense[_sparse[entity]] = std::move(component);
			return &_dense[_sparse[entity]];
		}

		uint32_t newIndex = static_cast<uint32_t>(_dense.size());
		_dense.emplace_back(std::move(component));
		_denseToEntity.push_back(entity);
		_sparse[entity] = newIndex;

		_needsSorting = true;
		return &_dense[newIndex];
	}

	void SortByEntityId()
	{
		if (!_needsSorting) [[likely]]
			return;

		std::vector<std::pair<Entity, uint32_t>> entityIndexPairs;
		for (uint32_t i = 0; i < _denseToEntity.size(); ++i)
		{
			entityIndexPairs.emplace_back(_denseToEntity[i], i);
		}
		std::sort(entityIndexPairs.begin(), entityIndexPairs.end());

		std::vector<TComponent> newDense;
		std::vector<Entity> newDenseToEntity;
		newDense.reserve(_dense.size());
		newDenseToEntity.reserve(_denseToEntity.size());

		for (uint32_t newIndex = 0; newIndex < entityIndexPairs.size(); ++newIndex)
		{
			Entity entity = entityIndexPairs[newIndex].first;
			uint32_t oldIndex = entityIndexPairs[newIndex].second;

			newDense.emplace_back(std::move(_dense[oldIndex]));
			newDenseToEntity.push_back(entity);
			_sparse[entity] = newIndex;
		}

		_dense = std::move(newDense);
		_denseToEntity = std::move(newDenseToEntity);
		_needsSorting = false;
	}

	TComponent* GetComponent(Entity entity)
	{
		if (entity >= _sparse.size()) [[unlikely]]
			return nullptr;

		uint32_t index = _sparse[entity];
		if (index == INVALID_INDEX) [[unlikely]]
			return nullptr;

		return &_dense[index];
	}

	void RemoveComponent(Entity entity)
	{
		if (entity >= _sparse.size()) [[unlikely]]
			return;

		uint32_t indexToRemove = _sparse[entity];
		if (indexToRemove == INVALID_INDEX) [[unlikely]]
			return;

		uint32_t lastIndex = static_cast<uint32_t>(_dense.size() - 1);

		if (indexToRemove != lastIndex)
		{
			_dense[indexToRemove] = std::move(_dense[lastIndex]);
			Entity lastEntity = _denseToEntity[lastIndex];
			_denseToEntity[indexToRemove] = lastEntity;
			_sparse[lastEntity] = indexToRemove;
		}

		_dense.pop_back();
		_denseToEntity.pop_back();
		_sparse[entity] = INVALID_INDEX;
	}

	auto begin()
	{
		SortByEntityId();
		return _dense.begin();
	}

	auto end()
	{
		return _dense.end();
	}

	const auto begin() const
	{
		return _dense.begin();
	}

	const auto end() const
	{
		return _dense.end();
	}

	const std::vector<Entity>& GetEntities() const
	{
		return _denseToEntity;
	}

	size_t Size() const
	{
		return _dense.size();
	}

	void Reserve(size_t capacity)
	{
		_dense.reserve(capacity);
		_denseToEntity.reserve(capacity);
	}
};