// PtrList.h
///////////////////////////////////////////////////////////////////////////////

#pragma once

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

	static MemoryManager<Node> nodeAllocator;

	size_t _size;
	Node *_begin, *_end;

	static void FreeNode(Node *node)
	{
		node->prev->next = node->next;
		node->next->prev = node->prev;
		nodeAllocator.Free(node);
	}

	PtrList(const PtrList &src); // no copy

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
			assert(_node && _node->next);
			_node = _node->next;
			return (*this);
		}
		iterator operator++ (int)  // postfix increment
		{
			assert(_node && _node->next);
			iterator tmp(_node);
			_node = _node->next;
			return tmp;
		}
		iterator& operator-- ()  // prefix decrement
		{
			assert(_node && _node->prev);
			_node = _node->prev;
			return (*this);
		}
		iterator operator-- (int)  // postfix decrement
		{
			assert(_node && _node->prev);
			iterator tmp(_node);
			_node = _node->prev;
			return tmp;
		}
		void operator = (const NullHelper *arg) // to allow iter=NULL
		{
			assert(NULL == arg);
			_node = NULL;
		}
	};


	class safe_iterator : public base_iterator  // increments node's reference counter
	{
	public:
		safe_iterator() : _node(NULL) {}
		explicit safe_iterator(Node *p)
		  : base_iterator(p)
		{
			++_node->ref_count;
		}
		~safe_iterator()
		{
			if( _node && 0 == (--_node->ref_count) && NULL == _node->ptr )
				FreeNode(_node);
		}

		safe_iterator& operator++ ()  // prefix increment
		{
			assert(_node && _node->next);
			if( 0 == (--_node->ref_count) && NULL == _node->ptr )
			{
				_node = _node->next;
				FreeNode(_node->prev);
			}
			else
			{
				_node = _node->next;
			}
			++_node->ref_count;
			return (*this);
		}
		safe_iterator& operator-- ()  // prefix decrement
		{
			assert(_node && _node->prev);
			if( 0 == (--_node->ref_count) && NULL == _node->ptr )
			{
				_node = _node->prev;
				FreeNode(_node->next);
			}
			else
			{
				_node = _node->prev;
			}
			++_node->ref_count;
			return (*this);
		}
		safe_iterator& operator= (const base_iterator& src)
		{
			if( _node && 0 == --_node->ref_count && !_node->ptr )
				FreeNode(_node);
			_node = src._node;
			if( _node ) _node->ref_count++;
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
			assert(_node && _node->prev);
			_node = _node->prev;
			return (*this);
		}
		reverse_iterator operator++ (int)  // postfix increment
		{
			assert(_node && _node->prev);
			reverse_iterator tmp(_node);
			_node = _node->prev;
			return tmp;
		}
		reverse_iterator& operator-- ()  // prefix decrement
		{
			assert(_node && _node->next);
			_node = _node->next;
			return (*this);
		}
		reverse_iterator operator-- (int)  // postfix decrement
		{
			assert(_node && _node->next);
			reverse_iterator tmp(_node);
			_node = _node->next;
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
	  , _begin(nodeAllocator.Alloc())
	  , _end(nodeAllocator.Alloc())
	{
		_begin->ptr       = (T*) -1;     // not NULL but invalid pointer
		_begin->ref_count = 0;
		_begin->next      = _end;
		_begin->prev      = NULL;
		_end->ptr         = (T*) -1;     // not NULL but invalid pointer
		_end->ref_count   = 0;
		_end->prev        = _begin;
		_end->next        = NULL;
	}

	~PtrList(void)
	{
		Node *tmp;
		while( _begin )
		{
			tmp = _begin;
			_begin = _begin->next;
			nodeAllocator.Free(tmp);
		}
	}

	void push_front(T *ptr)
	{
		Node *tmp = nodeAllocator.Alloc();
		tmp->ptr        = ptr;
		tmp->ref_count  = 0;
		tmp->prev       = _begin;
		tmp->next       = _begin->next;
		tmp->prev->next = tmp->next->prev = tmp;
		++_size;
	}

	void push_back(T *ptr)
	{
		Node *tmp = nodeAllocator.Alloc();
		tmp->ptr        = ptr;
		tmp->ref_count  = 0;
		tmp->prev       = _end->prev;
		tmp->next       = _end;
		tmp->prev->next = tmp->next->prev = tmp;
		++_size;
	}

	void erase(base_iterator &where)
	{
		assert(0 < _size);
		assert(0 == where._node->ref_count);  // use safe_erase to handle this
		assert(where._node != _begin);
		assert(where._node != _end);
		T *ptr = where._node->ptr;
		FreeNode(where._node);
		--_size;
	}

	void safe_erase(base_iterator &where)
	{
		assert(0 < _size);
		assert(0 <= where._node->ref_count);
		T *ptr = where._node->ptr;
		if( where._node->ref_count )
			where._node->ptr = 0;
		else
			FreeNode(where._node);
		--_size;
	}

	__inline iterator      begin() const { return iterator(_begin->next); }
	__inline base_iterator end()   const { return base_iterator(_end); }

	safe_iterator safe_begin() const { return safe_iterator(_begin->next); }

	reverse_iterator rbegin() const { return reverse_iterator(_end->prev); }
	base_iterator    rend()   const { return base_iterator(_begin); }

	bool empty() const
	{
		return _begin->next == _end;
	}

	T* front() const
	{
		assert(!empty());
		return _begin->next->ptr;
	}

	T* back()  const
	{
		assert(!empty());
		return _end->prev->ptr;
	}

	size_t size() const { return _size; } // FIXME
};

template <class T>
MemoryManager<typename PtrList<T>::Node>
	PtrList<T>::nodeAllocator;

///////////////////////////////////////////////////////////////////////////////
// end of file
