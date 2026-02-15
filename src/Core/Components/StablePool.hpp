#pragma once
#include <cassert>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

/// <summary>
/// A pool allocator with pointer-stable storage.
/// Allocates elements in fixed-size blocks. Pointers to elements remain valid
/// for the lifetime of the pool (blocks are never freed or moved).
/// Freed slots are recycled via an internal free list.
/// </summary>
/// <typeparam name="T">Element type.</typeparam>
/// <typeparam name="BlockSize">Number of elements per block. Must be a power of
/// two.</typeparam>
template <typename T, uint32_t BlockSize = 4096>
class StablePool
{
	static_assert(BlockSize > 0 && (BlockSize & (BlockSize - 1)) == 0, "BlockSize must be a power of two");

	static constexpr uint32_t BLOCK_SHIFT = []() constexpr
	{
		uint32_t shift = 0;
		uint32_t v = BlockSize;
		while (v > 1)
		{
			v >>= 1;
			++shift;
		}
		return shift;
	}();
	static constexpr uint32_t BLOCK_MASK = BlockSize - 1;

	struct Block
	{
		alignas(alignof(T)) unsigned char data[sizeof(T) * BlockSize];

		T& at(uint32_t localIdx) noexcept
		{
			return *std::launder(reinterpret_cast<T*>(data + sizeof(T) * localIdx));
		}

		const T& at(uint32_t localIdx) const noexcept
		{
			return *std::launder(reinterpret_cast<const T*>(data + sizeof(T) * localIdx));
		}

		T* ptr(uint32_t localIdx) noexcept
		{
			return std::launder(reinterpret_cast<T*>(data + sizeof(T) * localIdx));
		}
	};

	std::vector<std::unique_ptr<Block>> _blocks;
	std::vector<uint32_t> _freeIndices;
	uint32_t _capacity = 0;
	uint32_t _liveCount = 0;

	void ensureCapacity()
	{
		if (_capacity % BlockSize == 0)
		{
			_blocks.push_back(std::make_unique<Block>());
		}
	}

public:
	StablePool() = default;

	~StablePool()
	{
		// Note: We don't track which slots are alive, so we don't call destructors.
		// For non-trivial types ComponentArray is responsible for clearing before destruction.
	}

	/// <summary>
	/// Allocates a slot, placement-moves the value into it.
	/// Returns (global index, pointer to the element).
	/// </summary>
	std::pair<uint32_t, T*> allocate(T&& value)
	{
		uint32_t index;

		if (!_freeIndices.empty())
		{
			index = _freeIndices.back();
			_freeIndices.pop_back();
			(*this)[index] = std::move(value);
		}
		else
		{
			index = _capacity;
			ensureCapacity();
			// Placement new for first-time initialization
			T* slot = _blocks[index >> BLOCK_SHIFT]->ptr(index & BLOCK_MASK);
			new (slot) T(std::move(value));
			++_capacity;
		}

		++_liveCount;
		return {index, &(*this)[index]};
	}

	/// <summary>
	/// Marks a slot as free. The slot can be reused by a future allocate() call.
	/// Does NOT call the destructor.
	/// </summary>
	void deallocate(uint32_t index)
	{
		_freeIndices.push_back(index);
		--_liveCount;
	}

	T& operator[](uint32_t index) noexcept
	{
		return _blocks[index >> BLOCK_SHIFT]->at(index & BLOCK_MASK);
	}

	const T& operator[](uint32_t index) const noexcept
	{
		return _blocks[index >> BLOCK_SHIFT]->at(index & BLOCK_MASK);
	}

	T* at(uint32_t index) noexcept
	{
		return _blocks[index >> BLOCK_SHIFT]->ptr(index & BLOCK_MASK);
	}

	/// <summary>
	/// Pre-allocates the specified number of blocks ahead of time.
	/// </summary>
	void reserveBlocks(uint32_t numBlocks)
	{
		_blocks.reserve(numBlocks);
	}

	/// <summary>
	/// Returns the total number of live (non-free) elements.
	/// </summary>
	uint32_t liveCount() const noexcept
	{
		return _liveCount;
	}

	/// <summary>
	/// Returns the total number of slots ever allocated (high watermark).
	/// </summary>
	uint32_t capacity() const noexcept
	{
		return _capacity;
	}

	/// <summary>
	/// Returns the number of free recycled slots.
	/// </summary>
	uint32_t freeCount() const noexcept
	{
		return static_cast<uint32_t>(_freeIndices.size());
	}

	/// <summary>
	/// Returns the number of allocated blocks.
	/// </summary>
	uint32_t blockCount() const noexcept
	{
		return static_cast<uint32_t>(_blocks.size());
	}
};
