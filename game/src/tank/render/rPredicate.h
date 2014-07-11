#pragma once
#include "ObjectView.h"

#include <functional>

template <class RenderType>
class R_Predicate : public ObjectView
{
public:
	template <class F, class ...Args>
	R_Predicate(F && cond, Args && ...args)
		: _condition(std::forward<F>(cond))
		, _render(std::forward<Args>(args)...)
	{
	}
	
	// ObjectView
	virtual enumZOrder GetZ(const World &world, const GC_Actor &actor) const
	{
		return _condition(world, actor) ? _render.GetZ(world, actor) : Z_NONE;
	}
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override
	{
		_render.Draw(world, actor, dc);
	}
	
private:
	std::function<bool(const World &, const GC_Actor &)> _condition;
	RenderType _render;
};
