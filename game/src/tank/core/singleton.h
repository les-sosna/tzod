// singleton.h

#pragma once

///////////////////////////////////////////////////////////////////////////////
// dynamic singleton

template <class T>
class DynamicSingleton : private T
{
	static T* pInstance_;

	DynamicSingleton() // create object via CreateInstance
	{
		_ASSERT (NULL == pInstance_);
		pInstance_ = this;
	}

public:
	typedef T object_type;
	static T* Inst() { return pInstance_; }
	static T* CreateInstance() { return new DynamicSingleton<T>; }
	static void Destroy()
	{
		_ASSERT(pInstance_);
		delete pInstance_;
		pInstance_ = NULL;
	}


	virtual ~DynamicSingleton()
	{
		_ASSERT(NULL != pInstance_);
		pInstance_ = NULL;
	}
};

template <class T>
T* DynamicSingleton<T>::pInstance_ = NULL;

///////////////////////////////////////////////////////////////////////////////
// static singleton

template <class T, int index=0>
class StaticSingleton
{
	static T _inst;
	StaticSingleton() {};
public:
	typedef T object_type;
	static inline T& Inst() { return _inst; }
};

template <class T, int index>
T StaticSingleton<T,index>::_inst;


///////////////////////////////////////////////////////////////////////////////
// end of file
