// SafePtr.h

#pragma once

/////////////////////////////////////////////////////////////

//  -- notes --
// do not allow implicit conversion to T*
// do not allow implicit conversion from T*
// do not define any member functions except the constructors/destructor. use friend functions instead
// do not define operator bool
// do not define operator& because it will break compatibility with STL

template<class T>
class SafePtr
{
	class IfNotEmpty
	{
		void operator delete (void*) {} // it's private so we can't call delete ptr
	};

	struct InitFromNullHelper {};

	T *_ptr;

public:

	//
	// construction
	//
	SafePtr()
	  : _ptr(NULL)
	{
	}
	SafePtr(const InitFromNullHelper *f) // to allow implicit construction from NULL
	  : _ptr(NULL)
	{
		_ASSERT(NULL == f);
	}
	explicit SafePtr(T *f)
	  : _ptr(f)
	{
		if( _ptr )
		{
			_ptr->AddRef();
		}
	}
	SafePtr(const SafePtr &f) // overwrite default copy constructor
	  : _ptr(f._ptr)
	{
		if( _ptr )
		{
			_ptr->AddRef();
		}
	}
	template<class U>
	SafePtr(const SafePtr<U> &f) // initialize from any compatible type of safe pointer
	  : _ptr(GetRawPtr(f))
	{
		if( _ptr )
		{
			_ptr->AddRef();
		}
	}

	inline friend SafePtr WrapRawPtr(T *p)
	{
		return SafePtr(p);
	}

	template<class U>
	inline friend SafePtr<U> SafePtrCast(const SafePtr &src)
	{
		_ASSERT(!src._ptr || dynamic_cast<U*>(src._ptr));
		return WrapRawPtr(static_cast<U*>(src._ptr));
	}


	//
	// destruction
	//
	~SafePtr()
	{
		if( _ptr )
		{
			_ptr->Release();
		}
	}


	//
	// assignment
	//
	const SafePtr& operator = (T *p)
	{
		if( _ptr )
		{
			_ptr->Release();
		}
		_ptr = p;
		if( _ptr )
		{
			_ptr->AddRef();
		}
		return *this;
	}
	const SafePtr& operator = (const SafePtr &p) // overwrite default assignment operator
	{
		if( _ptr )
		{
			_ptr->Release();
		}
		_ptr = p._ptr;
		if( _ptr )
		{
			_ptr->AddRef();
		}
		return *this;
	}
	template<class U>
	const SafePtr& operator = (const SafePtr<U> &p)
	{
		if( _ptr )
		{
			_ptr->Release();
		}
		_ptr = GetRawPtr(p); // access private _ptr of another SafePtr
		if( _ptr )
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
	// Direct access access to _ptr
	//
	inline friend T* GetRawPtr(const SafePtr &r)
	{
		return r._ptr;
	}
	inline friend void SetRawPtr(SafePtr &r, T *p)
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
	template<class U>
	inline friend bool operator==(const SafePtr &l, const U *r)
	{
		return l._ptr == r;
	}
	template<class U>
	inline friend bool operator==(const U *l, const SafePtr &r)
	{
		return l == r._ptr;
	}
	template<class U>
	inline friend bool operator!=(const SafePtr &l, const U *r)
	{
		return l._ptr != r;
	}
	template<class U>
	inline friend bool operator!=(const U *l, const SafePtr &r)
	{
		return l != r._ptr;
	}
	template<class U> // for comparing safe pointers of different types
	bool operator==(const SafePtr<U> &r) const
	{
		return _ptr == GetRawPtr(r);
	}
	template<class U> // for comparing safe pointers of different types
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
