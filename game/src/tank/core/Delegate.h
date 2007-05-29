// Delegate.h

#pragma once

///////////////////////////////////////////////////////////////////////////////
// simple delegate implementation
// it don't support virtual functions!

#define INVOKE(d) ((d).inst()->*(d).func())

template<class F>
class Delegate
{
    struct blank {};         // base for all targets
    typedef F (blank::*mp);  // type pointer to member

    blank  *_inst;
    mp      _func;

public:
	Delegate()
	{
		_inst = NULL;
		_func = NULL;
	}

	blank* inst() const { return _inst; }
	mp     func() const { return _func; }

    template<class signature, class inst_type>
    void bind (signature pmf, inst_type *p_this)
    {
		_ASSERT(pmf);
		_ASSERT(p_this);
        typedef F (inst_type::*mem_fn);
        mem_fn tmp = static_cast<mem_fn>(pmf); // safe(!) cast
        _func = reinterpret_cast<mp>(tmp);     // unsafe, but it has been checked above
		_inst = reinterpret_cast<blank*>(p_this);
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
// end of file
