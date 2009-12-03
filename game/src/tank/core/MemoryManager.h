// MemoryManager.h
///////////////////////////////////////////

#pragma once

///////////////////////////////////////////////////////////////////

template
<
	class T,
	size_t block_size = 128
>
class MemoryManager1
{
	union BlankObject
	{
		BlankObject *pNext;
		char object[sizeof(T)];
	};

	std::vector<BlankObject *> _memoryBlocks;
	BlankObject *_freeBlock;

#ifndef NDEBUG
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
	MemoryManager1()
	  : _freeBlock(NULL)
#ifndef NDEBUG
	  , _allocatedCount(0)
	  , _allocatedPeak(0)
#endif
	{
	}

	~MemoryManager1()
	{
		for( size_t i = 0; i < _memoryBlocks.size(); i++ )
			::free(_memoryBlocks[i]);

#ifndef NDEBUG
		assert(0 == _allocatedCount);
	//	TRACE("MemoryManager<%s>: peak allocation is %u", typeid(T).name(), _allocatedPeak);
#endif
	}

	T* Alloc(void)
	{
#ifndef NDEBUG
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
		assert(_allocatedCount--);
		((BlankObject*) p)->pNext = _freeBlock;
		_freeBlock = (BlankObject*) p;
	}
};

///////////////////////////////////////////////////////////////////////////////


template
<
	class T,
	size_t block_size = 128
>
class MemoryPool
{
	struct BlankObject
	{
		union
		{
			BlankObject *_next;
			char _data[sizeof(T)];
		};
		struct Block *_block;
	};

	struct Block
	{
		Block *_prev;
		Block *_next;
		Block *_prevFree;
		Block *_nextFree;

		BlankObject *_blanks;
		BlankObject *_free;
		size_t _used;

		Block()
		  : _prev(NULL)
		  , _next(NULL)
		  , _prevFree(NULL)
		  , _nextFree(NULL)
		  , _blanks((BlankObject *) malloc(sizeof(BlankObject) * block_size))
		  , _free(_blanks)
		  , _used(0)
		{
			// link together newly allocated blanks
			BlankObject *tmp(_blanks);
			BlankObject *end(_blanks + block_size - 1);
			while( tmp != end )
			{
				tmp->_block = this;
				tmp->_next = tmp + 1;
				++tmp;
			}
			end->_block = this;
			end->_next = NULL;
		}

		~Block()
		{
			free(_blanks);
		}

		BlankObject* Alloc()
		{
			assert(_free);
			BlankObject *tmp = _free;
			_free = _free->_next;
			++_used;
			return tmp;
		}

		void Free(BlankObject *p)
		{
			assert(this == p->_block);
			assert(_used > 0);
#ifndef NDEBUG
			memset(p, 0xdb, sizeof(T));
#endif
			p->_next = _free;
			_free = p;
			--_used;
		}
	}; // struct block


	Block *_blocks;
	Block *_freeBlock;

#ifndef NDEBUG
	size_t _allocatedCount;
	size_t _allocatedPeak;
#endif

	void Grow()
	{
		if( _blocks )
		{
			assert(!_blocks->_prev);
			_blocks->_prev = new Block();
			_blocks->_prev->_next = _blocks;
			_blocks = _blocks->_prev;
		}
		else
		{
			_blocks = new Block();
		}

		if( _freeBlock )
		{
			assert(!_freeBlock->_prevFree);
			_blocks->_nextFree = _freeBlock;
			_freeBlock->_prevFree = _blocks;
		}
		_freeBlock = _blocks;
	}

public:
	MemoryPool()
	  : _blocks(NULL)
#ifndef NDEBUG
	  , _allocatedCount(0)
	  , _allocatedPeak(0)
#endif
	{
	}

	~MemoryPool()
	{
		while( _blocks )
		{
			Block *tmp = _blocks;
			_blocks = _blocks->_next;
			delete tmp;
		}

#ifndef NDEBUG
		assert(0 == _allocatedCount);
		printf("MemoryPool<%s>: peak allocation is %u\n", typeid(T).name(), _allocatedPeak);
#endif
	}

	T* Alloc()
	{
#ifndef NDEBUG
		if( ++_allocatedCount > _allocatedPeak )
			_allocatedPeak = _allocatedCount;
#endif

		if( !_freeBlock )
		{
			Grow();
		}

		BlankObject *result = _freeBlock->Alloc();
		assert(_freeBlock == result->_block);

		if( !_freeBlock->_free )
		{
			// no more free blanks in this block; remove block from free list
			assert(!_freeBlock->_prevFree);
			Block *tmp = _freeBlock;
			_freeBlock = tmp->_nextFree;
			if( _freeBlock )
				_freeBlock->_prevFree = NULL;
			tmp->_nextFree = NULL;
		}

		return (T *) result->_data;
	}

	void Free(void* p)
	{
		assert(_allocatedCount--);

		Block *block = ((BlankObject*) p)->_block;
		if( !block->_free )
		{
			// block just became free
			assert(!block->_prevFree);
			assert(!block->_nextFree);

			if( _freeBlock )
			{
				assert(!_freeBlock->_prevFree);
				_freeBlock->_prevFree = block;
				block->_nextFree = _freeBlock;
			}
			_freeBlock = block;
		}

		block->Free((BlankObject *) p);

		if( 0 == block->_used )
		{
			// free unused block

			if( block == _blocks )
				_blocks = _blocks->_next;
			if( block->_prev )
				block->_prev->_next = block->_next;
			if( block->_next )
				block->_next->_prev = block->_prev;

			if( block == _freeBlock )
				_freeBlock = _freeBlock->_nextFree;
			if( block->_prevFree )
				block->_prevFree->_nextFree = block->_nextFree;
			if( block->_nextFree )
				block->_nextFree->_prevFree = block->_prevFree;

			delete block;
		}
	}
};


#define MemoryManager	MemoryPool

///////////////////////////////////////////////////////////////////////////////
// end of file
