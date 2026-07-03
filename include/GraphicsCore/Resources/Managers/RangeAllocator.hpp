#pragma once

#include "HalcyonExport.hpp"
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <optional>
#include <vector>

class HALCYON_API RangeAllocator
{
public:
	struct Range
	{
		uint32_t offset;
		uint32_t size;
	};

	RangeAllocator() = default;

	void reset(uint32_t capacity)
	{
		_capacity = capacity;
		_totalFree = capacity;
		_free.clear();
		if (capacity > 0) _free.push_back({0, capacity});
	}

	std::optional<uint32_t> allocate(uint32_t count)
	{
		if (count == 0) return std::nullopt;
		for (auto it = _free.begin(); it != _free.end(); ++it)
		{
			if (it->size >= count)
			{
				uint32_t offset = it->offset;
				it->offset += count;
				it->size -= count;
				if (it->size == 0) _free.erase(it);
				_totalFree -= count;
				return offset;
			}
		}
		return std::nullopt;
	}

	void free(uint32_t offset, uint32_t count)
	{
		if (count == 0) return;

		_totalFree += count;

		// Kept sorted by offset so a freed block only ever touches its two neighbours.
		auto it = std::lower_bound(_free.begin(), _free.end(), offset,
		                           [](const Range& range, uint32_t offset) { return range.offset < offset; });
		it = _free.insert(it, {offset, count});

		auto next = std::next(it);
		if (next != _free.end() && it->offset + it->size == next->offset)
		{
			it->size += next->size;
			_free.erase(next);
		} 
		if (it != _free.begin())
		{
			auto prev = std::prev(it);
			if (prev->offset + prev->size == it->offset)
			{
				prev->size += it->size;
				_free.erase(it);
			}
		}
	}

	uint32_t capacity() const
	{
		return _capacity;
	}

	uint32_t totalFree() const
	{
		return _totalFree;
	}

	const std::vector<Range>& freeRanges() const
	{
		return _free;
	}

private:
	uint32_t _capacity = 0; // one element size, not "bytes"
	uint32_t _totalFree = 0;
	std::vector<Range> _free;
};
