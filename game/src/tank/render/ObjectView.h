#pragma once
#include <gc/Z.h>

class DrawingContext;
class GC_Actor;
class World;

struct ObjectZFunc
{
	virtual enumZOrder GetZ(const World &world, const GC_Actor &actor) const = 0;
	virtual ~ObjectZFunc() {}
};

struct ObjectRFunc
{
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const = 0;
	virtual ~ObjectRFunc() {}
};
