#pragma once

#include <vector>
#include <cassert>
#include <cstddef>

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
      , _dataTail(-1)
      , _freeTail(-1)
      , _eraseIt(false)
	{}

    T* at(id_type id) const
    {
        assert(id._id >= 0 && id._id < (int) _data.size());
        assert(InvalidPtr() != _data[id._id].ptr);
        return _data[id._id].ptr;
    }

    id_type begin() const { return _dataTail; }
	id_type end()   const { return -1; }

	void erase(const id_type id)
	{
        assert(id._id >= 0 && id._id < (int) _data.size());
        assert(InvalidPtr() != _data[id._id].ptr);
        if (id != _it)
        {
            FreeNode(id._id);
        }
        else
        {
            assert(!_eraseIt);
            _eraseIt = true;
        }
    }

	bool empty() const { return !_size; }

    template<class F>
    void for_each(const F &f)
    {
        assert(!_dbgInLoop);
        assert(_dbgInLoop = true);
        assert(!_eraseIt);
        for( _it = begin(); _it != end(); )
        {
            f(_it, at(_it));
            id_type n = next(_it);
            if( _eraseIt )
            {
                FreeNode(_it._id);
                _eraseIt = false;
            }
            _it = n;
        }
        assert(!(_dbgInLoop = false));
    }

    id_type insert(T *p)
    {
        id_type where = -1 != _freeTail ? _freeTail : (int) _data.size();
        insert(p, where);
        return where;
    }

    void insert(T *p, id_type where)
    {
        assert(InvalidPtr() != p);
        assert(where._id >= 0);

        // allocate new free nodes and put them to free list at tail
        int numIds = (int) _data.size();
        if (where._id >= numIds)
        {
            _data.resize(where._id + 1);
            if (-1 != _freeTail)
                _data[_freeTail].next = numIds;
            for (int i = numIds; i <= where._id; ++i)
            {
                assert(_data[i].ptr = InvalidPtr());
                _data[i].prev = (i == numIds) ? _freeTail : (i-1);
                _data[i].next = (i == where._id) ? -1 : (i+1);
            }
            _freeTail = where._id;
        }

        // move node from free to data
        assert(InvalidPtr() == _data[where._id].ptr);
        _data[where._id].ptr = p;
        MoveNode(where._id, _freeTail, _dataTail);
        ++_size;
    }

    id_type next(id_type id) const
    {
        assert(id._id >= 0 && id._id < (int) _data.size());
        assert(InvalidPtr() != _data[id._id].ptr);
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
	int _dataTail;
    int _freeTail;
    id_type _it;
    bool _eraseIt;
#ifndef NDEBUG
    bool _dbgInLoop = false;
    T* InvalidPtr() const { return (T*) this; }
#endif

    void MoveNode(const int nodeId, int &tailFrom, int &tailTo)
    {
        Node &node = _data[nodeId];
        // remove from
        if (-1 != node.next)
            _data[node.next].prev = node.prev;
        if (-1 != node.prev)
            _data[node.prev].next = node.next;
        if (tailFrom == nodeId)
            tailFrom = node.prev;
        // append to
        node.prev = tailTo;
        node.next = -1;
        if (-1 != tailTo)
            _data[tailTo].next = nodeId;
        tailTo = nodeId;
    }

    void FreeNode(const int nodeId)
    {
        // remove node from data list
        assert(InvalidPtr() != _data[nodeId].ptr);
        assert(_data[nodeId].ptr = InvalidPtr());
        assert(_size > 0);
        --_size;
        MoveNode(nodeId, _dataTail, _freeTail);
    }
};
