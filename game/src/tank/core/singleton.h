// singleton.h

#pragma once

///////////////////////////////////////////////////////////////////////////////
// static singleton

template<class T, int index=0>
class StaticSingleton
{
	static T _inst;
	StaticSingleton() {};
public:
	typedef T object_type;
	static inline T& Inst() { return _inst; }
};

template<class T, int index>
T StaticSingleton<T,index>::_inst;


///////////////////////////////////////////////////////////////////////////////
// end of file
