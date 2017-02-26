#pragma once
#include <gc/Z.h>

class RenderContext;
class GC_Actor;
class World;

struct ObjectZFunc
{
	virtual enumZOrder GetZ(const World &world, const GC_Actor &actor) const = 0;
	virtual ~ObjectZFunc() {}
};

struct ObjectRFunc
{
	virtual void Draw(const World &world, const GC_Actor &actor, RenderContext &rc) const = 0;
	virtual ~ObjectRFunc() {}
};
