#pragma once

#include <vector>
#include <cstdint>
#include <algorithm>

using Entity = uint32_t;

// ActiveEntitySet: compact sparse-set implementation optimized for fast contains/add/remove and dense iteration.
// - dense: stores active entity ids compactly for cache-friendly iteration
// - sparse: maps an entity id to its index in dense (or a garbage value)
// Complexity: insert/erase/contains O(1), iteration O(n) over dense
class ActiveEntitySet
{
public:
    ActiveEntitySet() = default;

    bool contains(Entity entity) const noexcept
    {
        if (entity >= _sparse.size()) return false;
        size_t idx = _sparse[entity];
        return idx < _dense.size() && _dense[idx] == entity;
    }

    void insert(Entity entity)
    {
        if (contains(entity)) return;
        if (entity >= _sparse.size()) _sparse.resize(entity + 1, static_cast<size_t>(-1));
        _sparse[entity] = _dense.size();
        _dense.push_back(entity);
    }

    void erase(Entity entity)
    {
        if (!contains(entity)) return;
        size_t idx = _sparse[entity];
        Entity last = _dense.back();
        _dense[idx] = last;
        _sparse[last] = idx;
        _dense.pop_back();
        _sparse[entity] = static_cast<size_t>(-1);
    }

    void clear() noexcept
    {
        _dense.clear();
        _sparse.clear();
    }

    size_t size() const noexcept { return _dense.size(); }

    const std::vector<Entity>& dense() const noexcept { return _dense; }

    auto begin() const noexcept { return _dense.begin(); }
    auto end() const noexcept { return _dense.end(); }

private:
    std::vector<Entity> _dense;
    std::vector<size_t> _sparse;
};
