#pragma once
#include <limits>
#include <deque>

template <typename TComponent>
class ComponentArray
{
private:
	std::deque<TComponent> _dense;
	std::vector<Entity> _denseToEntity;
	std::vector<uint32_t> _freeIndices;

	std::vector<uint32_t> _sparse;
	static constexpr uint32_t INVALID_INDEX = std::numeric_limits<uint32_t>::max();

public:
	TComponent* addComponent(Entity entity, TComponent&& component)
	{
		if (entity >= _sparse.size())
		{
			_sparse.resize(entity + 1, INVALID_INDEX);
		}

		if (_sparse[entity] != INVALID_INDEX)
		{
			uint32_t index = _sparse[entity];
			_dense[index] = std::move(component);
			return &_dense[index];
		}

		uint32_t newIndex;
		if (!_freeIndices.empty())
		{
			newIndex = _freeIndices.back();
			_freeIndices.pop_back();
			_dense[newIndex] = std::move(component);
			_denseToEntity[newIndex] = entity;
		}
		else
		{
			newIndex = static_cast<uint32_t>(_dense.size());
			_dense.emplace_back(std::move(component));
			_denseToEntity.push_back(entity);
		}

		_sparse[entity] = newIndex;
		return &_dense[newIndex];
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

		return &_dense[index];
	}

	void removeComponent(Entity entity)
	{
		if (entity >= _sparse.size()) [[unlikely]]
			return;

		uint32_t indexToRemove = _sparse[entity];
		if (indexToRemove == INVALID_INDEX) [[unlikely]]
			return;

		_freeIndices.push_back(indexToRemove);
		_sparse[entity] = INVALID_INDEX;
	}

	auto begin()
	{
		return _dense.begin();
	}

	auto end()
	{
		return _dense.end();
	}

	const std::vector<Entity>& getEntities() const
	{
		return _denseToEntity;
	}

	size_t size() const
	{
		return _dense.size() - _freeIndices.size();
	}

	void reserve(size_t capacity)
	{
		_denseToEntity.reserve(capacity);
	}
};