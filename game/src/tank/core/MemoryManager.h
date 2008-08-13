// MemoryManager.h
///////////////////////////////////////////

#pragma once

#include <stdlib.h>
#include <vector>

///////////////////////////////////////////////////////////////////

template
<
	class T,
	size_t block_size = 128
>
class MemoryManager
{
	union BlankObject
	{
		BlankObject *pNext;
		char object[sizeof(T)];
	};

	std::vector<BlankObject *> _memoryBlocks;
	BlankObject *_freeBlock;

#ifdef _DEBUG
	size_t _allocatedCount;
	size_t _allocatedPeak;
#endif

	void Grow()
	{
		BlankObject *begin = (BlankObject *) malloc(sizeof(BlankObject) * block_size);
		BlankObject *end = begin + block_size;
		_memoryBlocks.push_back(begin);

		// link together newly allocated blanks
		do {
			begin->pNext = begin+1;
		} while( ++begin != end );

		(--end)->pNext = _freeBlock;
		_freeBlock = _memoryBlocks.back();
	}

public:
	MemoryManager()
	  : _freeBlock(NULL)
#ifdef _DEBUG
	  , _allocatedCount(0)
	  , _allocatedPeak(0)
#endif
	{
	}

	~MemoryManager()
	{
		for( size_t i = 0; i < _memoryBlocks.size(); i++ )
			::free(_memoryBlocks[i]);

#ifdef _DEBUG
		_ASSERT(0 == _allocatedCount);
	//	TRACE("MemoryManager<%s>: peak allocation is %u\n", typeid(T).name(), _allocatedPeak);
#endif
	}

	T* Alloc(void)
	{
#ifdef _DEBUG
		if( ++_allocatedCount > _allocatedPeak )
			_allocatedPeak = _allocatedCount;
#endif

		if( !_freeBlock ) Grow();
		BlankObject* tmp = _freeBlock;
		_freeBlock = _freeBlock->pNext;
		return (T*) tmp;
	}

	void Free(T* p)
	{
		_ASSERT(_allocatedCount--);
		((BlankObject*) p)->pNext = _freeBlock;
		_freeBlock = (BlankObject*) p;
	}
};

///////////////////////////////////////////////////////////////////////////////
// end of file
