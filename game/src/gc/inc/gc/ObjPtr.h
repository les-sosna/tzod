#pragma once

#include <cassert>

template <class T>
class ObjPtr
{
	typedef void (*ObjFinalizerProc) (void *);
	T *_ptr;
public:
	ObjPtr() : _ptr(nullptr) {}
	ObjPtr(T *f)
		: _ptr(f)
	{
		if( _ptr ) ++((unsigned int *) _ptr)[-1];
	}
	ObjPtr(const ObjPtr &f)
		: _ptr(f._ptr)
	{
		if( _ptr ) ++((unsigned int *) _ptr)[-1];
	}

	~ObjPtr()
	{
		if( _ptr && 0 == --((unsigned int *) _ptr)[-1] )
			(*(ObjFinalizerProc*) _ptr)((unsigned int *) _ptr - 1);
	}

	const ObjPtr& operator = (T *p)
	{
		if( p )
			++*((unsigned int *) p - 1);
		if( _ptr && 0 == --((unsigned int *) _ptr)[-1] )
			(*(ObjFinalizerProc*) _ptr)((unsigned int *) _ptr - 1);
		_ptr = p;
		return *this;
	}

	operator T* () const
	{
		return (_ptr && (((unsigned int *)_ptr)[-1] & 0x80000000)) ? _ptr : nullptr;
	}

	T* operator -> () const
	{
		assert(*this);
		return _ptr;
	}
};

template<class U, class T>
inline U* PtrDynCast(T *src)
{
	assert(!src || ObjPtr<T>(src));
	return dynamic_cast<U*>(src);
}

template<class U, class T>
inline U* PtrDynCast(const ObjPtr<T> &src)
{
	return dynamic_cast<U*>(src.operator T*());
}

template<class U, class T>
inline U* PtrCast(const ObjPtr<T> &src)
{
	assert(!src || PtrDynCast<U>(src));
	return static_cast<U*>(src.operator T*());
}
