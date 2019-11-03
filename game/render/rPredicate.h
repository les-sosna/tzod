#pragma once
#include "inc/render/ObjectView.h"
#include <functional>
#include <type_traits>

template <class ZFuncType>
class Z_Predicate : public ObjectZFunc
{
public:
	template <class F, class ...Args>
	Z_Predicate(F && cond, Args && ...args)
		: _condition(std::forward<F>(cond))
		, _zfunc(std::forward<Args>(args)...)
	{
	}

	enumZOrder GetZ(const World &world, const GC_MovingObject &mo) const override
	{
		return _condition(world, mo) ? _zfunc.GetZ(world, mo) : Z_NONE;
	}

private:
	std::function<bool(const World &, const GC_MovingObject &)> _condition;
	ZFuncType _zfunc;
};

template <class F>
class _Not
{
public:
	_Not(F && f) : _f(std::forward<F>(f)) {}
	template <class ...Args>
	auto operator()(Args && ...args) ->  decltype(std::declval<F>()(std::forward<Args>(args)...))
	{
		return !_f(std::forward<Args>(args)...);
	}
private:
	F _f;
};

template <class F>
_Not<F> Not(F && f)
{
	return _Not<F>(std::forward<F>(f));
}


template <class F1, class F2>
class _And
{
public:
	_And(F1 && f1, F2 && f2)
		: _f1(std::forward<F1>(f1))
		, _f2(std::forward<F2>(f2))
	{}
	template <class ...Args>
	bool operator()(const Args& ...args)
	{
		return _f1(args...) && _f2(args...);
	}
private:
	F1 _f1;
	F2 _f2;
};

template <class F1, class F2>
_And<F1, F2> And(F1 && f1, F2 && f2)
{
	return _And<F1, F2>(std::forward<F1>(f1), std::forward<F2>(f2));
}
