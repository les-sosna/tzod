// MemoryManager.h

#pragma once

#include <cassert>
#include <cstring> // memset
#include <new>
#include <typeinfo>
#ifndef NDEBUG
#include <cstdio>  // printf
#endif

template
<
	class T,
	size_t extra_bytes = 0,
	size_t block_size = 128
>
class MemoryPool
{
    struct Block;

	struct BlankObject
	{
		union
		{
			struct
			{
				BlankObject *_next;
				BlankObject *_prev; // to enable allocation with specified ID
			};
			char _data[sizeof(T) + extra_bytes];
		};
		Block *_block;
#ifndef NDEBUG
		bool _dbgBusy;
#endif
	};

	struct Block
	{
		Block *_prevFree;
		Block *_nextFree;

		BlankObject _blanks[block_size];
		BlankObject *_firstFreeBlank;
		size_t _used;

		size_t _thisBlockIdx;

		Block(size_t idx)
		  : _prevFree(nullptr)
		  , _nextFree(nullptr)
		  , _firstFreeBlank(_blanks)
		  , _used(0)
		  , _thisBlockIdx(idx)
		{
#ifndef NDEBUG
            memset(_blanks, 0xdb, sizeof(_blanks));
#endif

			// link together empty object blanks
			BlankObject *tmp(_blanks);
			BlankObject *end(_blanks + block_size - 1);
			tmp->_prev = nullptr;
			while( tmp != end )
			{
				tmp->_block = this;
				tmp->_next = tmp + 1;
				tmp->_next->_prev = tmp;
#ifndef NDEBUG
                tmp->_dbgBusy = false;
#endif
				++tmp;
			}
			end->_block = this;
			end->_next = nullptr;
#ifndef NDEBUG
            end->_dbgBusy = false;
#endif
		}

		~Block()
		{
			assert(!_used);
		}

		BlankObject* Alloc(unsigned int id = 0)
		{
			assert(_firstFreeBlank);
			assert(!_firstFreeBlank->_prev);
			++_used;
			if( !id )
			{
				BlankObject *tmp = _firstFreeBlank;
#ifndef NDEBUG
                assert(!tmp->_dbgBusy);
                tmp->_dbgBusy = true;
#endif
				_firstFreeBlank = _firstFreeBlank->_next;
				if( _firstFreeBlank )
					_firstFreeBlank->_prev = nullptr;
				return tmp;
			}
			else
			{
				assert(!"not implemented");
				return nullptr;
			}
		}

		void Free(BlankObject *p)
		{
			assert(this == p->_block);
			assert(_used > 0);
#ifndef NDEBUG
            memset(p, 0xdb, sizeof(T) + extra_bytes);
            assert(p->_dbgBusy);
            p->_dbgBusy = false;
#endif
			if( _firstFreeBlank )
				_firstFreeBlank->_prev = p;
			p->_next = _firstFreeBlank;
			p->_prev = nullptr;
			_firstFreeBlank = p;
			--_used;
		}

	private:
		Block();
		Block(Block&);
		Block& operator =(Block&);
	}; // struct block

	struct BlockPtr
	{
		Block *_block;
		size_t _nextEmptyIdx; // not a pointer to enable realloc
	};


	BlockPtr *_blocks;
	size_t _blockCount;
	size_t _firstEmptyIdx;
	Block *_freeBlock;


#ifndef NDEBUG
	size_t _allocatedCount;
	size_t _allocatedPeak;
#endif

public:
	MemoryPool()
	  : _blocks((BlockPtr*) malloc(sizeof(BlockPtr)))
	  , _blockCount(1)
	  , _firstEmptyIdx(0)
	  , _freeBlock(nullptr)
#ifndef NDEBUG
	  , _allocatedCount(0)
	  , _allocatedPeak(0)
#endif
	{
		if( !_blocks )
			throw std::bad_alloc();
		_blocks->_block = nullptr;
		_blocks->_nextEmptyIdx = 1; // out of range
	}

	~MemoryPool()
	{
#ifndef NDEBUG
        assert(0 == _allocatedCount);
        for( size_t i = 0; i < _blockCount; ++i )
            assert(!_blocks[i]._block);
        printf("MemoryPool<%s>: peak allocation is %zu\n", typeid(T).name(), _allocatedPeak);
#endif
		free(_blocks);
	}

	void* Alloc()
	{
#ifndef NDEBUG
        if( ++_allocatedCount > _allocatedPeak )
            _allocatedPeak = _allocatedCount;
#endif

		if( !_freeBlock )
		{
			// grow if no empty blocks available
			if( _firstEmptyIdx == _blockCount )
			{
				_blocks = (BlockPtr *) realloc(_blocks, sizeof(BlockPtr) * _blockCount * 2);
				if( !_blocks )
					throw std::bad_alloc();
				for( size_t i = _blockCount; i < _blockCount * 2; ++i )
				{
					_blocks[i]._block = nullptr;
					_blocks[i]._nextEmptyIdx = i + 1; // last is out of range
				}
				_blockCount *= 2;
			}

			_freeBlock = new Block(_firstEmptyIdx);

			size_t tmp = _firstEmptyIdx;
			_firstEmptyIdx = _blocks[tmp]._nextEmptyIdx;

			assert(!_blocks[tmp]._block);
			_blocks[tmp]._block = _freeBlock;
#ifndef NDEBUG
            _blocks[tmp]._nextEmptyIdx = -1;
#endif
		}

		BlankObject *result = _freeBlock->Alloc();
		assert(_freeBlock == result->_block);

		// no more free blanks in this block; remove block from free list
		if( !_freeBlock->_firstFreeBlank )
		{
			assert(!_freeBlock->_prevFree);
			Block *tmp = _freeBlock;
			_freeBlock = tmp->_nextFree;
			if( _freeBlock )
				_freeBlock->_prevFree = nullptr;
			tmp->_nextFree = nullptr;
		}

		return (T *) result->_data;
	}

	void Free(void* p)
	{
		assert(_allocatedCount--);

		Block *block = ((BlankObject*) p)->_block;
		if( !block->_firstFreeBlank )
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

			if( block == _freeBlock )
				_freeBlock = _freeBlock->_nextFree;
			if( block->_prevFree )
				block->_prevFree->_nextFree = block->_nextFree;
			if( block->_nextFree )
				block->_nextFree->_prevFree = block->_prevFree;

			_blocks[block->_thisBlockIdx]._block = nullptr;
			_blocks[block->_thisBlockIdx]._nextEmptyIdx = _firstEmptyIdx;
			_firstEmptyIdx = block->_thisBlockIdx;

			delete block;
		}
	}

	unsigned int GetAllocID(void *p) const
	{
		return 0;
	}
};


#ifdef NDEBUG
#define _DBG_FILL_FREE_PATTERN
#else
#define _DBG_FILL_FREE_PATTERN(p, sz) memset(p, 0xfe, sz)
#endif


#define DECLARE_POOLED_ALLOCATION(cls)          \
private:                                        \
    static MemoryPool<cls, sizeof(int)> __pool; \
	static void __fin(void *allocated)          \
	{                                           \
		__pool.Free(allocated);                 \
	}                                           \
public:                                         \
    void* operator new(size_t count)            \
    {                                           \
        assert(sizeof(cls) == count);           \
		void *_ptr = __pool.Alloc();            \
		*(unsigned int*) _ptr = 0x80000000;     \
        return (unsigned int*) _ptr + 1;        \
    }                                           \
    void operator delete(void *p)               \
    {                                           \
        _DBG_FILL_FREE_PATTERN(p, sizeof(cls)); \
		unsigned int&cnt(*((unsigned int*)p-1));\
		cnt &= 0x7fffffff;                      \
		typedef void (*ObjFinalizerProc) (void *); \
		if( !cnt )                              \
			__pool.Free((unsigned int*) p - 1); \
		else                                    \
			*(ObjFinalizerProc*) p = __fin;     \
    }

#define IMPLEMENT_POOLED_ALLOCATION(cls)        \
    MemoryPool<cls, sizeof(int)> cls::__pool;




///////////////////////////////////////////////////////////////////////////////
// end of file
