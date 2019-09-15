#pragma once
#include <gc/Z.h>

class RenderContext;
class GC_MovingObject;
class World;

struct ObjectZFunc
{
	virtual enumZOrder GetZ(const World &world, const GC_MovingObject &mo) const = 0;
	virtual ~ObjectZFunc() {}
};

struct ObjectRFunc
{
	virtual void Draw(const World &world, const GC_MovingObject &mo, RenderContext &rc) const = 0;
	virtual ~ObjectRFunc() {}
};
