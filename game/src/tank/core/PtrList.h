// PtrList.h
//////////////////////////////////////////////////////////////////////

#pragma once

///////////////////////////////////

template <class object_type>
class PtrList
{
	struct _node
	{
		object_type *ptr;
		int ref_count;			// счетчик safe-итераторов
		_node *prev, *next;
	};

	static MemoryManager<_node> nodeAllocator;

	size_t mysize;
	_node *mybegin, *myend;

	static void _free_node( _node *node_ptr )
	{
		node_ptr->prev->next = node_ptr->next;
		node_ptr->next->prev = node_ptr->prev;
        nodeAllocator.free(node_ptr);
	}

public:

	/////////////////////////////////////////////
	// iterators

	class base_iterator
	{
		friend class PtrList;

	protected:
		_node *node_ptr;

	public:
		base_iterator() : node_ptr(NULL) {}
		explicit base_iterator(_node *p) : node_ptr(p) {}

		object_type* operator * () const { return node_ptr->ptr; }

		bool operator != (const base_iterator& it) const
		{
			return node_ptr != it.node_ptr;
		}
		bool operator == (const base_iterator& it) const
		{
			return node_ptr == it.node_ptr;
		}
	};

	class iterator : public base_iterator
	{
	public:
		iterator() {}
		explicit iterator(_node *p) : base_iterator(p) {}

		iterator& operator++ ()  // prefix increment
		{
			_ASSERT(node_ptr && node_ptr->next);
            node_ptr = node_ptr->next;
			return (*this);
		}
		iterator operator++ (int)  // postfix increment
		{
			_ASSERT(node_ptr && node_ptr->next);
			iterator tmp(node_ptr);
            node_ptr = node_ptr->next;
			return tmp;
		}
		iterator& operator-- ()  // prefix decrement
		{
			_ASSERT(node_ptr && node_ptr->prev);
            node_ptr = node_ptr->prev;
			return (*this);
		}
		iterator operator-- (int)  // postfix decrement
		{
			_ASSERT(node_ptr && node_ptr->prev);
			iterator tmp(node_ptr);
            node_ptr = node_ptr->prev;
			return tmp;
		}
	};


	class safe_iterator : public base_iterator	// обеспечивает подсчет ссылок
	{
	public:
		safe_iterator() : node_ptr(NULL) {}
		explicit safe_iterator(_node *p) : base_iterator(p)
		{
			++node_ptr->ref_count;
		}
		~safe_iterator()
		{
            if( node_ptr && 0 == (--node_ptr->ref_count) && NULL == node_ptr->ptr )
				_free_node(node_ptr);
		}

		safe_iterator& operator++ ()  // prefix increment
		{
			_ASSERT(node_ptr && node_ptr->next);
            if( 0 == (--node_ptr->ref_count) && NULL == node_ptr->ptr )
			{
	            node_ptr = node_ptr->next;
				_free_node(node_ptr->prev);
			}
			else
			{
				node_ptr = node_ptr->next;
			}
			++node_ptr->ref_count;
			return (*this);
		}
		safe_iterator& operator-- ()  // prefix decrement
		{
			_ASSERT(node_ptr && node_ptr->prev);
            if( 0 == (--node_ptr->ref_count) && NULL == node_ptr->ptr )
			{
	            node_ptr = node_ptr->prev;
				_free_node(node_ptr->next);
			}
			else
			{
				node_ptr = node_ptr->prev;
			}
			++node_ptr->ref_count;
			return (*this);
		}
		safe_iterator& operator= (const base_iterator& src)
		{
            if( node_ptr && 0 == (--node_ptr->ref_count) && NULL == node_ptr->ptr )
				_free_node(node_ptr);
			node_ptr = src.node_ptr;
			if( node_ptr ) node_ptr->ref_count++;
			return (*this);
		}
	};

	class reverse_iterator : public base_iterator
	{
	public:
		reverse_iterator() {}
		explicit reverse_iterator(_node *p) : base_iterator(p) {}

		reverse_iterator& operator++ ()  // prefix increment
		{
			_ASSERT(node_ptr && node_ptr->prev);
            node_ptr = node_ptr->prev;
			return (*this);
		}
		reverse_iterator operator++ (int)  // postfix increment
		{
			_ASSERT(node_ptr && node_ptr->prev);
			reverse_iterator tmp(node_ptr);
            node_ptr = node_ptr->prev;
			return tmp;
		}
		reverse_iterator& operator-- ()  // prefix decrement
		{
			_ASSERT(node_ptr && node_ptr->next);
            node_ptr = node_ptr->next;
			return (*this);
		}
		reverse_iterator operator-- (int)  // postfix decrement
		{
			_ASSERT(node_ptr && node_ptr->next);
			reverse_iterator tmp(node_ptr);
            node_ptr = node_ptr->next;
			return tmp;
		}
	};

	/////////////////////////////////////////////

	PtrList(void)
	{
		mysize  = 0;
		mybegin = nodeAllocator.allocate();
		myend   = nodeAllocator.allocate();
		mybegin->ptr       = (object_type*) -1;		// not NULL but invalid pointer
		mybegin->ref_count = 0;
		mybegin->next      = myend;
		mybegin->prev      = NULL;
		myend->ptr         = (object_type*) -1;		// not NULL but invalid pointer
		myend->ref_count   = 0;
		myend->prev        = mybegin;
		myend->next        = NULL;
	}

	PtrList(const PtrList &src)
	{
		mysize  = src.mysize;
		mybegin = nodeAllocator.allocate();
		myend   = nodeAllocator.allocate();
		mybegin->ptr       = (object_type*) -1;		// not NULL but invalid pointer
		mybegin->ref_count = 0;
		mybegin->next      = myend;
		mybegin->prev      = NULL;
		myend->ptr         = (object_type*) -1;		// not NULL but invalid pointer
		myend->ref_count   = 0;
		myend->prev        = mybegin;
		myend->next        = NULL;

		for( iterator it = src.begin(); it != src.end(); ++it )
            if(*it) push_back(*it);
	}

	~PtrList(void)
	{
		_node *tmp;
		while( mybegin )
		{
			tmp = mybegin;
			mybegin = mybegin->next;
			nodeAllocator.free(tmp);
		}
	}

	void push_front(object_type *ptr)
	{
		_node *tmp = nodeAllocator.allocate();
		tmp->ptr        = ptr;
		tmp->ref_count  = 0;
		tmp->prev       = mybegin;
		tmp->next       = mybegin->next;
		tmp->prev->next = tmp->next->prev = tmp;
		++mysize;
	}

	void push_back(object_type *ptr)
	{
		_node *tmp = nodeAllocator.allocate();
		tmp->ptr        = ptr;
		tmp->ref_count  = 0;
		tmp->prev       = myend->prev;
		tmp->next       = myend;
		tmp->prev->next = tmp->next->prev = tmp;
		++mysize;
	}

	void erase(base_iterator &where)
	{
		_ASSERT(0 <  mysize);
		_ASSERT(0 == where.node_ptr->ref_count);
		_free_node(where.node_ptr);
		--mysize;
	}

	void safe_erase(base_iterator &where)
	{
		_ASSERT(0 <  mysize);
		_ASSERT(0 <= where.node_ptr->ref_count);
		if( where.node_ptr->ref_count )
			where.node_ptr->ptr = 0;
		else
			_free_node(where.node_ptr);
		--mysize;
	}

	iterator      begin() const { return iterator(mybegin->next); }
	base_iterator end()   const { return base_iterator(myend); }

	safe_iterator safe_begin() const { return safe_iterator(mybegin->next); }

	reverse_iterator rbegin() const { return reverse_iterator(myend->prev); }
	base_iterator    rend()   const { return base_iterator(mybegin); }


	size_t size() const { return mysize; }
	object_type *front() const { return mybegin->ptr; }
	object_type *back()  const { return myend->ptr; }
};

template <class object_type>
MemoryManager<typename PtrList<object_type>::_node>
	PtrList<object_type>::nodeAllocator;

///////////////////////////////////////////////////////////////////////////////
// end of file
