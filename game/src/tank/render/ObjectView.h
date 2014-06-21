#pragma once
class DrawingContext;
class GC_Actor;
struct ObjectView
{
	virtual void Draw(const GC_Actor &actor, DrawingContext &dc, bool editorMode) const = 0;
	virtual ~ObjectView() {}
};
