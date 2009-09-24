// ComPtr.h

#pragma once


inline IUnknown* ComPtrAssignHelper(IUnknown** pp, IUnknown* lp)
{
	if( NULL == pp )
		return NULL;
	if( NULL != lp )
		lp->AddRef();
	if( *pp )
		(*pp)->Release();
	*pp = lp;
	return lp;
}

inline IUnknown* ComQIPtrAssignHelper(IUnknown** pp, IUnknown* lp, REFIID riid)
{
	if( NULL == pp )
		return NULL;
	IUnknown* pTemp = *pp;
	*pp = NULL;
	if( NULL != lp )
		lp->QueryInterface(riid, (void**)pp);
	if( pTemp )
		pTemp->Release();
	return *pp;
}


// ComPtrBase provides the basis for all other smart pointers
// The other smart pointers add their own constructors and operators
template <class T>
class ComPtrBase
{
protected:
	ComPtrBase() throw(): p(NULL) {}
	ComPtrBase(int nNull) throw(): p(NULL)
	{
		assert(nNull == 0);
		(void)nNull;
	}
	ComPtrBase(T* lp) throw()
	{
		p = lp;
		if( NULL != p )
			p->AddRef();
	}
public:
	typedef T _PtrClass;
	class NoAddRefReleaseOnComPtr : public T
	{
	private:
		STDMETHOD_(ULONG, AddRef)()=0;
		STDMETHOD_(ULONG, Release)()=0;
	};

	~ComPtrBase() throw()
	{
		if( p )
			p->Release();
	}
	operator T*() const throw()
	{
		return p;
	}
	T& operator*() const
	{
		assert(p);
		return *p;
	}
	T** operator&() throw()
	{
		//The assert on operator& usually indicates a bug.  If this is really
		//what is needed, however, take the address of the p member explicitly.
		assert(!p);
		return &p;
	}
	NoAddRefReleaseOnComPtr* operator->() const throw()
	{
		assert(p);
		return static_cast<NoAddRefReleaseOnComPtr *>(p);
	}
	bool operator!() const throw()
	{
		return (NULL == p);
	}
	bool operator<(T* pT) const throw()
	{
		return p < pT;
	}
	bool operator!=(T* pT) const
	{
		return !operator==(pT);
	}
	bool operator==(T* pT) const throw()
	{
		return p == pT;
	}

	// Release the interface and set to NULL
	void Release() throw()
	{
		T* tmp = p;
		if( tmp )
		{
			p = NULL;
			tmp->Release();
		}
	}
	// Compare two objects for equivalence
	bool IsEqualObject(IUnknown* pOther) throw()
	{
		if (p == NULL && pOther == NULL)
			return true;	// They are both NULL objects

		if (p == NULL || pOther == NULL)
			return false;	// One is NULL the other is not

		ComPtr<IUnknown> punk1;
		ComPtr<IUnknown> punk2;
		p->QueryInterface(__uuidof(IUnknown), (void**)&punk1);
		pOther->QueryInterface(__uuidof(IUnknown), (void**)&punk2);
		return punk1 == punk2;
	}
	// Attach to an existing interface (does not AddRef)
	void Attach(T* p2) throw()
	{
		if( p )
			p->Release();
		p = p2;
	}
	// Detach the interface (does not Release)
	T* Detach() throw()
	{
		T* pt = p;
		p = NULL;
		return pt;
	}
	HRESULT CopyTo(T** ppT) throw()
	{
		assert(ppT);
		if( NULL == ppT )
			return E_POINTER;
		*ppT = p;
		if( p )
			p->AddRef();
		return S_OK;
	}
	template <class Q>
	HRESULT QueryInterface(Q** pp) const throw()
	{
		assert(pp);
		return p->QueryInterface(__uuidof(Q), (void**)pp);
	}
	T* p;
};

template <class T>
class ComPtr : public ComPtrBase<T>
{
public:
	ComPtr() throw()
	{
	}
	ComPtr(int nNull) throw() :
		ComPtrBase<T>(nNull)
	{
	}
	ComPtr(T* lp) throw() :
		ComPtrBase<T>(lp)
	{
	}
	ComPtr(const ComPtr<T>& lp) throw() :
		ComPtrBase<T>(lp.p)
	{
	}
	T* operator =(T* lp) throw()
	{
		if( *this != lp )
		{
			return static_cast<T*>(ComPtrAssignHelper((IUnknown**)&p, lp));
		}
		return *this;
	}
	template <typename Q>
	T* operator =(const ComPtr<Q>& lp) throw()
	{
		if( !IsEqualObject(lp) )
		{
			return static_cast<T*>(ComQIPtrAssignHelper((IUnknown**)&p, lp, __uuidof(T)));
		}
		return *this;
	}
	T* operator =(const ComPtr<T>& lp) throw()
	{
		if( *this != lp )
		{
			return static_cast<T*>(ComPtrAssignHelper((IUnknown**)&p, lp));
		}
		return *this;
	}
};

// end of file
