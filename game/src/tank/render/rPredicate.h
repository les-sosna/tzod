#pragma once
#include "ObjectView.h"
#include <functional>

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
	
	virtual enumZOrder GetZ(const World &world, const GC_Actor &actor) const override
	{
		return _condition(world, actor) ? _zfunc.GetZ(world, actor) : Z_NONE;
	}
	
private:
	std::function<bool(const World &, const GC_Actor &)> _condition;
	ZFuncType _zfunc;
};
