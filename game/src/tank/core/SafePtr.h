// SafePtr.h

#pragma once

/////////////////////////////////////////////////////////////

/*
template <class T> class SafePtr
{
	union
	{
		T*     _object;
		size_t _objId;
	};

public:
	inline operator T* () const
	{
		return _object;
	}

	inline T* operator-> () const
	{
		_ASSERT(NULL != _object);
		return _object;
	}

	inline T*       p() const { return _object; }
	inline bool empty() const { return NULL == _object; }


public:
	SafePtr()
	{
		_object = NULL;
	}

	~SafePtr()
	{
		_ASSERT(NULL == _object);
	}

	inline void Release(bool kill = false)
	{
		if( NULL == _object ) return;
		if( kill ) _object->Kill();
		_object->Release();
		_object = NULL;
	}

	inline void operator= (T *obj)
	{
		if( _object )
			_object->Release();

		if( _object = obj )
			_object->AddRef();
	}

	inline void operator= (const SafePtr<T> &src)
	{
		*this = src._object;
	}

	inline T* create()	// create instance with default constructor
	{
		_ASSERT( NULL == _object );
		*this = new T;
		return _object;
	}

	void RawSet(T* obj)
	{
		_object = obj;
	}

	size_t GetId() const
	{
		return _objId;
	}
	void SetId(size_t id)
	{
		_objId = id;
	}
};
*/



///////////////////////////////////////////////////////////////////////////////
// 
//

//  -- notes --
// do not allow implicit convertion to T*
// do not define any member functions except the constructors/destructor. use friend functions instead
// do not define operator bool
// do not define operator& because it will break compatibility with STL

template <class T>
class SafePtr
{
    T *_ptr;
    class IfNotEmpty
    {
        void operator delete (void*) {} // it's private so we can't call delete ptr
    };

public:

    //
    // construction
    //
    SafePtr()
    {
        _ptr = NULL;
    }
    SafePtr(T *f)
    {
        if( _ptr = f )
        {
            _ptr->AddRef();
        }
    }
    SafePtr(const SafePtr &f)
    {
        if( _ptr = GetRawPtr(f) )
        {
            _ptr->AddRef();
        }
    }
    template <class U>
    SafePtr(const SafePtr<U> &f) // initialize from another type of safe pointer
    {
        if( _ptr = GetRawPtr(f) )
        {
            _ptr->AddRef();
        }
    }


    //
    // destruction
    //
    ~SafePtr()
    {
        if( _ptr ) _ptr->Release();
    }


    //
    // assignment
    //
    const SafePtr& operator = (T *p)
    {
        if( _ptr ) _ptr->Release();
        if( _ptr = p )
        {
            _ptr->AddRef();
        }
        return *this;
    }
    const SafePtr& operator = (const SafePtr &p)
    {
        if( _ptr ) _ptr->Release();
        if( _ptr = p._ptr )
        {
            _ptr->AddRef();
        }
        return *this;
    }
    template <class U>
    const SafePtr& operator = (const SafePtr<U> &p)
    {
        if( _ptr ) _ptr->Release();
        if( _ptr = GetRawPtr(p) )
        {
            _ptr->AddRef();
        }
        return *this;
    }


    //
    // dereference
    //
    T* operator -> () const
    {
        _ASSERT(_ptr);
        return _ptr;
    }
//    T& operator * () const
//    {
//        _ASSERT(_ptr);
//        return *_ptr;
//    }


    //
    // Direct access accsess to _ptr
    //
    friend T* GetRawPtr(const SafePtr &r)
    {
        return r._ptr;
    }
    friend void SetRawPtr(SafePtr &r, T *p)
    {
        r._ptr = p;
    }


    //
    // Equality and Inequality
    //
    operator const IfNotEmpty* () const // to allow if(ptr), if(!ptr)
    {
        return reinterpret_cast<const IfNotEmpty*>(_ptr);
    }
    template <class U>
    inline friend bool operator==(const SafePtr &l, const U *r)
    {
        return l._ptr == r;
    }
    template <class U>
    inline friend bool operator==(const U *l, const SafePtr &r)
    {
        return l == r._ptr;
    }
    template <class U>
    inline friend bool operator!=(const SafePtr &l, const U *r)
    {
        return l._ptr != r;
    }
    template <class U>
    inline friend bool operator!=(const U *l, const SafePtr &r)
    {
        return l != r._ptr;
    }
    template <class U> // for comparing safe pointers of different types
    bool operator==(const SafePtr<U> &r) const
    {
        return _ptr == GetRawPtr(r);
    }
    template <class U> // for comparing safe pointers of different types
    bool operator!=(const SafePtr<U> &r) const
    {
        return _ptr != GetRawPtr(r);
    }
};

template<> SafePtr<void>::~SafePtr() {} // to allow instantiation of SafePtr<void>


//
// abstract base class with reference counting functionality
//
class RefCounted
{
    int _refCount;

#ifdef _DEBUG
    struct tracker
    {
        int _count;
        ~tracker() // destructor will be called by runtime at the end of program
        {
            _ASSERT(0 == _count); // leaks!
        }
    };
    static tracker _tracker;
#endif

protected:
    RefCounted();
    virtual ~RefCounted() = 0;

public:
    void AddRef()
    {
        ++_refCount;
    }
    void Release()
    {
        if( 0 == --_refCount )
            delete this;
    }
};


///////////////////////////////////////////////////////////////////////////////
// end of file
