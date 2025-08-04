#pragma once
#include <limits>
#include <vector>

#include "../Entitys/EntityManager.h"

template <typename T>
class ComponentArray
{
private:
	std::vector<T> dense;              // ���������� � ����������� ������
	std::vector<Entity> denseToEntity; // dense_index -> entity

	// ����������� ������ ��� O(1) �������
	std::vector<uint32_t> sparse; // entity -> dense_index
	static constexpr uint32_t INVALID_INDEX = std::numeric_limits<uint32_t>::max();

	bool needsSorting = false;


public:
	T* AddComponent(Entity entity, T&& component)
	{
		if (entity >= sparse.size())
		{
			sparse.resize(entity + 1, INVALID_INDEX);
		}

		if (sparse[entity] != INVALID_INDEX)
		{
			dense[sparse[entity]] = std::move(component);
			return &dense[sparse[entity]];
		}

		uint32_t newIndex = static_cast<uint32_t>(dense.size());
		dense.emplace_back(std::move(component));
		denseToEntity.push_back(entity);
		sparse[entity] = newIndex;

		needsSorting = true; // �������� ��� ����� ����������
		return &dense[newIndex];
	}

	// ��������� dense ������ �� Entity ID ��� ����������������� �������
	void SortByEntityId()
	{
		if (!needsSorting) return;

		// ������� ���� (entity, index) ��� ����������
		std::vector<std::pair<Entity, uint32_t>> entityIndexPairs;
		for (uint32_t i = 0; i < denseToEntity.size(); ++i)
		{
			entityIndexPairs.emplace_back(denseToEntity[i], i);
		}

		// ��������� �� Entity ID
		std::sort(entityIndexPairs.begin(), entityIndexPairs.end());

		// ����������������� dense � denseToEntity
		std::vector<T> newDense;
		std::vector<Entity> newDenseToEntity;
		newDense.reserve(dense.size());
		newDenseToEntity.reserve(denseToEntity.size());

		for (uint32_t newIndex = 0; newIndex < entityIndexPairs.size(); ++newIndex)
		{
			Entity entity = entityIndexPairs[newIndex].first;
			uint32_t oldIndex = entityIndexPairs[newIndex].second;

			newDense.emplace_back(std::move(dense[oldIndex]));
			newDenseToEntity.push_back(entity);
			sparse[entity] = newIndex; // ��������� sparse mapping
		}

		dense = std::move(newDense);
		denseToEntity = std::move(newDenseToEntity);
		needsSorting = false;
	}

	T* GetComponent(Entity entity)
	{
		if (entity >= sparse.size()) [[unlikely]]
			return nullptr;

		uint32_t index = sparse[entity];
		if (index == INVALID_INDEX) [[unlikely]]
			return nullptr;

		return &dense[index]; 
	}

	// O(1) �������� � ����������� ���������
	void RemoveComponent(Entity entity)
	{
		if (entity >= sparse.size()) [[unlikely]]
			return;

		uint32_t indexToRemove = sparse[entity];
		if (indexToRemove == INVALID_INDEX) [[unlikely]]
			return;

		uint32_t lastIndex = static_cast<uint32_t>(dense.size() - 1);

		if (indexToRemove != lastIndex)
		{
			dense[indexToRemove] = std::move(dense[lastIndex]);
			Entity lastEntity = denseToEntity[lastIndex];
			denseToEntity[indexToRemove] = lastEntity;
			sparse[lastEntity] = indexToRemove;
		}

		dense.pop_back();
		denseToEntity.pop_back();
		sparse[entity] = INVALID_INDEX;
	}

	auto begin()
	{
		SortByEntityId();
		return dense.begin();
	}
	auto end()
	{
		return dense.end();
	}
	const auto begin() const
	{
		return dense.begin();
	}
	const auto end() const
	{
		return dense.end();
	}

	const std::vector<Entity>& GetEntities() const
	{
		return denseToEntity;
	}

	size_t Size() const
	{
		return dense.size();
	}
	void Reserve(size_t capacity)
	{
		dense.reserve(capacity);
		denseToEntity.reserve(capacity);
	}
};