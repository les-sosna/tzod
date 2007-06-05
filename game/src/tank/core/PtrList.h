// PtrList.h
///////////////////////////////////////////////////////////////////////////////

#pragma once

///////////////////////////////////

template <class object_type>
class PtrList
{
	struct Node
	{
		object_type *ptr;
		int ref_count;       // cout of safe iterators using this node
		Node *prev, *next;
	};

	static MemoryManager<Node> nodeAllocator;

	size_t _size;
	Node *_begin, *_end;

	static void FreeNode(Node *node)
	{
		node->prev->next = node->next;
		node->next->prev = node->prev;
        nodeAllocator.free(node);
	}

public:

	/////////////////////////////////////////////
	// iterators

	class base_iterator
	{
		friend class PtrList;

	protected:
		Node *_node;

	public:
		base_iterator() : _node(NULL) {}
		explicit base_iterator(Node *p) : _node(p) {}

		object_type* operator * () const { return _node->ptr; }

		bool operator != (const base_iterator& it) const
		{
			return _node != it._node;
		}
		bool operator == (const base_iterator& it) const
		{
			return _node == it._node;
		}
	};

	class iterator : public base_iterator
	{
	public:
		iterator() {}
		explicit iterator(Node *p) : base_iterator(p) {}

		iterator& operator++ ()  // prefix increment
		{
			_ASSERT(_node && _node->next);
            _node = _node->next;
			return (*this);
		}
		iterator operator++ (int)  // postfix increment
		{
			_ASSERT(_node && _node->next);
			iterator tmp(_node);
            _node = _node->next;
			return tmp;
		}
		iterator& operator-- ()  // prefix decrement
		{
			_ASSERT(_node && _node->prev);
            _node = _node->prev;
			return (*this);
		}
		iterator operator-- (int)  // postfix decrement
		{
			_ASSERT(_node && _node->prev);
			iterator tmp(_node);
            _node = _node->prev;
			return tmp;
		}
	};


	class safe_iterator : public base_iterator	// обеспечивает подсчет ссылок
	{
	public:
		safe_iterator() : _node(NULL) {}
		explicit safe_iterator(Node *p) : base_iterator(p)
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
			_ASSERT(_node && _node->next);
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
			_ASSERT(_node && _node->prev);
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
			_ASSERT(_node && _node->prev);
            _node = _node->prev;
			return (*this);
		}
		reverse_iterator operator++ (int)  // postfix increment
		{
			_ASSERT(_node && _node->prev);
			reverse_iterator tmp(_node);
            _node = _node->prev;
			return tmp;
		}
		reverse_iterator& operator-- ()  // prefix decrement
		{
			_ASSERT(_node && _node->next);
            _node = _node->next;
			return (*this);
		}
		reverse_iterator operator-- (int)  // postfix decrement
		{
			_ASSERT(_node && _node->next);
			reverse_iterator tmp(_node);
            _node = _node->next;
			return tmp;
		}
	};

	/////////////////////////////////////////////

	PtrList(void)
	{
		_size  = 0;
		_begin = nodeAllocator.allocate();
		_end   = nodeAllocator.allocate();
		_begin->ptr       = (object_type*) -1;     // not NULL but invalid pointer
		_begin->ref_count = 0;
		_begin->next      = _end;
		_begin->prev      = NULL;
		_end->ptr         = (object_type*) -1;     // not NULL but invalid pointer
		_end->ref_count   = 0;
		_end->prev        = _begin;
		_end->next        = NULL;
	}

	PtrList(const PtrList &src)
	{
		_size  = src._size;
		_begin = nodeAllocator.allocate();
		_end   = nodeAllocator.allocate();
		_begin->ptr       = (object_type*) -1;     // not NULL but invalid pointer
		_begin->ref_count = 0;
		_begin->next      = _end;
		_begin->prev      = NULL;
		_end->ptr         = (object_type*) -1;     // not NULL but invalid pointer
		_end->ref_count   = 0;
		_end->prev        = _begin;
		_end->next        = NULL;

		for( iterator it = src.begin(); it != src.end(); ++it )
            if(*it) push_back(*it);
	}

	~PtrList(void)
	{
		Node *tmp;
		while( _begin )
		{
			tmp = _begin;
			_begin = _begin->next;
			nodeAllocator.free(tmp);
		}
	}

	void push_front(object_type *ptr)
	{
		Node *tmp = nodeAllocator.allocate();
		tmp->ptr        = ptr;
		tmp->ref_count  = 0;
		tmp->prev       = _begin;
		tmp->next       = _begin->next;
		tmp->prev->next = tmp->next->prev = tmp;
		++_size;
	}

	void push_back(object_type *ptr)
	{
		Node *tmp = nodeAllocator.allocate();
		tmp->ptr        = ptr;
		tmp->ref_count  = 0;
		tmp->prev       = _end->prev;
		tmp->next       = _end;
		tmp->prev->next = tmp->next->prev = tmp;
		++_size;
	}

	void erase(base_iterator &where)
	{
		_ASSERT(0 < _size);
		_ASSERT(0 == where._node->ref_count);
		FreeNode(where._node);
		--_size;
	}

	void safe_erase(base_iterator &where)
	{
		_ASSERT(0 < _size);
		_ASSERT(0 <= where._node->ref_count);
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

	bool empty() const { return 0 == _size; }
	object_type *front() const { return _begin->ptr; }
	object_type *back()  const { return _end->ptr; }
};

template <class object_type>
MemoryManager<typename PtrList<object_type>::Node>
	PtrList<object_type>::nodeAllocator;

///////////////////////////////////////////////////////////////////////////////
// end of file
