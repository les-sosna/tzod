// Delegate.h

#pragma once

///////////////////////////////////////////////////////////////////////////////
// simple delegate implementation
// it don't support virtual functions!

#define INVOKE(d) ((d).inst()->*(d).func())

namespace delegate_detail
{
	struct blank {};
}

template<class F>
class Delegate
{
	typedef F (delegate_detail::blank::*MemFnPtr);

	delegate_detail::blank     *_inst;
	MemFnPtr   _func;

public:
	Delegate()
	  : _inst(NULL)
	  , _func(NULL)
	{
	}

	template <class U>
	Delegate(Delegate<U> const &src)
	  : _inst(src.inst())
	  , _func(src.func())
	{
	}


	delegate_detail::blank*   inst() const { return _inst; }
	MemFnPtr                  func() const { return _func; }

	template<class MemFnType, class InstType>
	void bind(MemFnType pmf, InstType *inst)
	{
		assert(pmf && inst);
		typedef F (InstType::*MemFnCompat);
		MemFnCompat tmp = static_cast<MemFnCompat>(pmf); // safe(!) cast
		_func = reinterpret_cast<MemFnPtr>(tmp);         // unsafe, but it has been checked above
		_inst = reinterpret_cast<delegate_detail::blank*>(inst);
	}

	void clear()
	{
		_inst = NULL;
		_func = NULL;
	}

	operator bool () const
	{
		return NULL != _inst;
	}
};


///////////////////////////////////////////////////////////////////////////////
// parameter adapter

template<class ParamType>
class DelegateAdapter1
{
public:
	ParamType  param;
	Delegate<void(ParamType)>  eventOnEvent;

	void OnEvent()
	{
		if( eventOnEvent )
			INVOKE(eventOnEvent) (param);
	}

	explicit DelegateAdapter1(const ParamType &p)
	  : param(p)
	{
	}
};

///////////////////////////////////////////////////////////////////////////////


template<class FF, class InstType>
Delegate<FF> CreateDelegate(FF (InstType::*fn), InstType *inst)
{
	Delegate<FF> d;
	d.bind(fn, inst);
	return d;
}


///////////////////////////////////////////////////////////////////////////////
// end of file
