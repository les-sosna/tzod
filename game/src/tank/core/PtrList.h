// PtrList.h

#pragma once

#include <vector>

template <class T>
class PtrList
{
public:
    struct id_type
    {
        id_type() : _id(-1) {}
        bool operator==(id_type other) const { return _id == other._id; }
        bool operator!=(id_type other) const { return _id != other._id; }
        bool operator<(id_type other) const { return _id < other._id; }
        
    private:
        friend class PtrList<T>;
        id_type(int id) : _id(id) {}
        int _id;
    };

	PtrList(void)
	  : _size(0)
      , _last(-1)
      , _free(0)
	{}
    
    T* at(id_type id) const { return _data[id._id].ptr; }

    id_type begin() const { return _last; }
	id_type end()   const { return -1; /*(int) _data.size();*/ }

	void erase(const id_type id)
	{
        Node &node = _data[id._id];
        if ((int) _data.size() != node.next)
            _data[node.next].prev = node.prev;
        if (-1 != node.prev)
            _data[node.prev].next = node.next;
        if (id == _last)
            _last = node.prev;
        node.next = _free;
        node.prev = -1;
        _free = id._id;
        --_size;
	}
    
	bool empty() const { return !_size; }
    
    id_type insert(T *p)
    {
        if (_free == _data.size())
            _data.push_back(Node{nullptr, -1, _free + 1});
        int id = _free;
        _free = _data[id].next;
        _data[id].ptr = p;
        _data[id].prev = _last;
        _data[id].next = (int) _data.size();
        if (_size)
            _data[_last].next = id;
        _last = id;
        ++_size;
        return id;
    }
    
    id_type next(id_type id) const
    {
        return _data[id._id].prev;
    }

	size_t size() const { return _size; }

private:
	struct Node
	{
		T *ptr;
		int prev;
        int next;
	};
    
    std::vector<Node> _data;
    
	size_t _size;
	int _last;
    int _free;
    
	void FreeNode(int id)
	{
        Node &node = _data[id];
        
        // link siblings together
		_data[node.prev].next = node.next;
		_data[node.next].prev = node.prev;
        
        // put node to the free list
        node.prev = -1;
        node.next = _free;
        _free = id;
	}
    
	PtrList(const PtrList &) = delete;

};

// end of file
