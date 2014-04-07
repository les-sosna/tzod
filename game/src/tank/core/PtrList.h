// PtrList.h
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <core/MemoryManager.h>

///////////////////////////////////

template <class T>
class PtrList
{
	struct Node
	{
		T *ptr;
		int ref_count;       // count of safe iterators which use this node
		Node *prev, *next;
	};

	static MemoryPool<Node> nodeAllocator;

	size_t _size;
	mutable Node _begin, _end;

	static void FreeNode(Node *node)
	{
		node->prev->next = node->next;
		node->next->prev = node->prev;
		nodeAllocator.Free(node);
	}

	PtrList(const PtrList &); // no copy

public:

	/////////////////////////////////////////////
	// iterators

	class base_iterator
	{
		friend class PtrList;

	protected:
		Node *_node;
		class IfHelper
		{
			void operator delete (void*) {} // it's private so we can't call delete ptr
		};

	public:
		base_iterator() : _node(NULL) {}
		explicit base_iterator(Node *p) : _node(p) {}

		T* operator * () const
		{
			return _node->ptr;
		}
		bool operator != (const base_iterator& it) const
		{
			return _node != it._node;
		}
		bool operator == (const base_iterator& it) const
		{
			return _node == it._node;
		}
		operator const IfHelper* () const // to allow if(iter), if(!iter)
		{
			return reinterpret_cast<const IfHelper*>(_node);
		}
	};

	class iterator : public base_iterator
	{
		struct NullHelper {};

	public:
		iterator() {}
		explicit iterator(Node *p) : base_iterator(p) {}

		iterator& operator++ ()  // prefix increment
		{
			assert(this->_node && this->_node->next);
			this->_node = this->_node->next;
			return (*this);
		}
		iterator operator++ (int)  // postfix increment
		{
			assert(this->_node && this->_node->next);
			iterator tmp(this->_node);
			this->_node = this->_node->next;
			return tmp;
		}
		iterator& operator-- ()  // prefix decrement
		{
			assert(this->_node && this->_node->prev);
			this->_node = this->_node->prev;
			return (*this);
		}
		iterator operator-- (int)  // postfix decrement
		{
			assert(this->_node && this->_node->prev);
			iterator tmp(this->_node);
			this->_node = this->_node->prev;
			return tmp;
		}
		void operator = (const NullHelper *arg) // to allow iter=NULL
		{
			assert(NULL == arg);
			this->_node = NULL;
		}
	};


	class safe_iterator : public base_iterator  // increments node's reference counter
	{
	public:
		explicit safe_iterator(Node *p)
		  : base_iterator(p)
		{
			++this->_node->ref_count;
		}
		~safe_iterator()
		{
			if( this->_node && 0 == (--this->_node->ref_count) && NULL == this->_node->ptr )
				FreeNode(this->_node);
		}

		safe_iterator& operator++ ()  // prefix increment
		{
			assert(this->_node && this->_node->next);
			if( 0 == (--this->_node->ref_count) && NULL == this->_node->ptr )
			{
				this->_node = this->_node->next;
				FreeNode(this->_node->prev);
			}
			else
			{
				this->_node = this->_node->next;
			}
			++this->_node->ref_count;
			return (*this);
		}
		safe_iterator& operator-- ()  // prefix decrement
		{
			assert(this->_node && this->_node->prev);
			if( 0 == (--this->_node->ref_count) && NULL == this->_node->ptr )
			{
				this->_node = this->_node->prev;
				FreeNode(this->_node->next);
			}
			else
			{
				this->_node = this->_node->prev;
			}
			++this->_node->ref_count;
			return (*this);
		}
		safe_iterator& operator= (const base_iterator& src)
		{
			if( this->_node && 0 == --this->_node->ref_count && !this->_node->ptr )
				FreeNode(this->_node);
			this->_node = src._node;
			if( this->_node )
				this->_node->ref_count++;
			return (*this);
		}
	};

	class reverse_iterator : public base_iterator
	{
	public:
		reverse_iterator() {}
		explicit reverse_iterator(Node *p) : base_iterator(p) {}

		reverse_iterator& operator++ ()  // prefix increment
		{
			assert(this->_node && this->_node->prev);
			this->_node = this->_node->prev;
			return (*this);
		}
		reverse_iterator operator++ (int)  // postfix increment
		{
			assert(this->_node && this->_node->prev);
			reverse_iterator tmp(this->_node);
			this->_node = this->_node->prev;
			return tmp;
		}
		reverse_iterator& operator-- ()  // prefix decrement
		{
			assert(this->_node && this->_node->next);
			this->_node = this->_node->next;
			return (*this);
		}
		reverse_iterator operator-- (int)  // postfix decrement
		{
			assert(this->_node && this->_node->next);
			reverse_iterator tmp(this->_node);
			this->_node = this->_node->next;
			return tmp;
		}

		operator iterator& ()
		{
			return *reinterpret_cast<iterator*>(this);
		}
	};

	/////////////////////////////////////////////

	PtrList(void)
	  : _size(0)
	{
		_begin.ptr       = (T*) -1;     // not NULL but invalid pointer
		_begin.ref_count = 0;
		_begin.next      = &_end;
		_begin.prev      = NULL;
		_end.ptr         = (T*) -1;     // not NULL but invalid pointer
		_end.ref_count   = 0;
		_end.prev        = &_begin;
		_end.next        = NULL;
	}

	~PtrList(void)
	{
		Node *tmp;
		while( _begin.next != &_end )
		{
			tmp = _begin.next;
			_begin.next = _begin.next->next;
			nodeAllocator.Free(tmp);
		}
	}

	void push_front(T *ptr)
	{
		Node *tmp = (Node *) nodeAllocator.Alloc();
		tmp->ptr        = ptr;
		tmp->ref_count  = 0;
		tmp->prev       = &_begin;
		tmp->next       = _begin.next;
		tmp->prev->next = tmp->next->prev = tmp;
		++_size;
	}

	void push_back(T *ptr)
	{
		Node *tmp = (Node *) nodeAllocator.Alloc();
		tmp->ptr        = ptr;
		tmp->ref_count  = 0;
		tmp->prev       = _end.prev;
		tmp->next       = &_end;
		tmp->prev->next = tmp->next->prev = tmp;
		++_size;
	}

	void erase(base_iterator &where)
	{
		assert(0 < _size);
		assert(0 == where._node->ref_count);  // use safe_erase to handle this
		assert(where._node != &_begin);
		assert(where._node != &_end);
		FreeNode(where._node);
		--_size;
	}

	void safe_erase(base_iterator &where)
	{
		assert(0 < _size);
		assert(0 <= where._node->ref_count);
		if( where._node->ref_count )
			where._node->ptr = 0;
		else
			FreeNode(where._node);
		--_size;
	}

    iterator      begin() const { return iterator(_begin.next); }
	base_iterator end()   const { return base_iterator(&_end); }

	safe_iterator safe_begin() const { return safe_iterator(_begin.next); }

	reverse_iterator rbegin() const { return reverse_iterator(_end.prev); }
	base_iterator    rend()   const { return base_iterator(&_begin); }

	bool empty() const
	{
		return _begin.next == &_end;
	}

	T* front() const
	{
		assert(!empty());
		return _begin.next->ptr;
	}

	T* back()  const
	{
		assert(!empty());
		return _end.prev->ptr;
	}

	size_t size() const { return _size; }


	size_t IndexOf(const T *p) const
	{
		size_t currentIdx = 0;
		for( iterator it = begin(); it != end(); ++it, ++currentIdx )
			if( *it == p )
				return currentIdx;
		return -1;
	}
	T* GetByIndex(size_t index) const
	{
		size_t currentIdx = 0;
		for( iterator it = begin(); it != end(); ++it, ++currentIdx )
			if( currentIdx == index )
				return *it;
		return NULL;
	}
};

template <class T>
MemoryPool<typename PtrList<T>::Node>
	PtrList<T>::nodeAllocator;

///////////////////////////////////////////////////////////////////////////////
// end of file
