#include <cassert>
#include <cstdint>
#include <deque>

class RingAllocator
{
public:
	explicit RingAllocator(uint32_t capacity)
		: _capacity(capacity)
		, _allocations(1)
	{}

	// returns offset
	uint32_t Allocate(uint32_t size, uint32_t alignment)
	{
		assert(!_allocations.empty());
		assert(alignment && !(alignment & (alignment - 1)));
		uint32_t alignedBegin = (_allocationEnd + alignment - 1) & ~(alignment - 1);
		if (alignedBegin + size > _capacity)
		{
			alignedBegin = 0;
			if (_allocations.back().size)
			{
				_allocations.back().size += _capacity - _allocationEnd + size;
			}
			else
			{
				_committed += _capacity - _allocationEnd;
				_allocations[_allocations.size() - 2].size += _capacity - _allocationEnd;
				_allocations.back().size = size;
			}
		}
		else
		{
			_allocations.back().size += alignedBegin + size - _allocationEnd;
		}
		assert(_allocations.back().size <= _capacity);
		_allocationEnd = alignedBegin + size;
		return alignedBegin;
	}

	// create new fence; free all overlapped and return the last overlapped fence value
	uint64_t CommitFreeOverlapped(uint64_t fenceValue)
	{
		assert(fenceValue);
		assert(_allocations.back().size);

		uint64_t overlappedFenceValue = 0;

		_committed += _allocations.back().size;
		while (_committed > _capacity)
		{
			assert(_allocations.size() > 1);
			overlappedFenceValue = _allocations.front().fenceValue;
			_committed -= _allocations.front().size;
			_allocations.pop_front();
		}

		_allocations.back().fenceValue = fenceValue;
		_allocations.emplace_back();
		return overlappedFenceValue;
	}

	uint32_t GetCapacity() const { return _capacity; }

private:
	uint32_t _capacity;
	uint32_t _committed = 0;
	uint32_t _allocationEnd = 0;
	struct Allocation
	{
		uint32_t size;
		uint64_t fenceValue;
	};
	std::deque<Allocation> _allocations;
};
