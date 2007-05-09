// MemoryManager.h
///////////////////////////////////////////

#pragma once

#include <stdlib.h>
#include <malloc.h>
#include <vector>

#include "core/debug.h"

///////////////////////////////////////////////////////////////////

template
<
	class T,
	size_t block_size = 128,
	size_t initial_blocks = 1
>
class MemoryManager
{
	union mem_block
	{
		mem_block *pNext;
		T object;
	};

	std::vector<mem_block *> _memory_blocks;
	mem_block *_freeBlock;

#ifdef _DEBUG
	size_t _allocated_count;
	size_t _allocated_peak;
#endif

	void grow()
	{
		mem_block *begin = (mem_block *) malloc(block_size * sizeof(mem_block));
		mem_block *end = begin + block_size;
		_memory_blocks.push_back(begin);
		do{ begin->pNext = begin+1; } while( ++begin != end );
		(--end)->pNext = _freeBlock;
		_freeBlock = _memory_blocks.back();
	}

public:
	MemoryManager(void)
	{
#ifdef _DEBUG
		_allocated_count = 0;
		_allocated_peak  = 0;
#endif
		_freeBlock = NULL;
		for( size_t i = 0; i < initial_blocks; i++ )
			grow();
	}

	~MemoryManager(void)
	{
		for( size_t i = 0; i < _memory_blocks.size(); i++ )
			::free(_memory_blocks[i]);

#ifdef _DEBUG
		_ASSERT(0 == _allocated_count);
		LOGOUT_3("MemoryManager<%s>: peak allocation is %u\n",
			typeid(T).name(), _allocated_peak);
#endif
	}

	T* allocate(void)
	{
#ifdef _DEBUG
		if( ++_allocated_count > _allocated_peak )
			_allocated_peak = _allocated_count;
#endif

		if( !_freeBlock ) grow();
		mem_block* tmp = _freeBlock;
		_freeBlock = _freeBlock->pNext;
		return (T*) tmp;
	}

	void free(T* p)
	{
		_ASSERT(_allocated_count--);
		((mem_block*) p)->pNext = _freeBlock;
		_freeBlock = (mem_block*) p;
	}
};

///////////////////////////////////////////////////////////////////////////////
// end of file
