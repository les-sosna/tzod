#include "inc/ctx/EditorContext.h"
#include <gc/World.h>
#include <gc/WorldCfg.h>
#include <MapFile.h>

EditorContext::EditorContext(int width, int height, FS::Stream *stream)
{
	assert(width >= 0 && height >= 0);
	RectRB bounds{ -width / 2, -height / 2, width / 2, height / 2 };

	if (stream)
	{
		MapFile mf(*stream, false);

		int width = 0;
		int height = 0;
		if (!mf.getMapAttribute("width", width) ||
			!mf.getMapAttribute("height", height))
		{
			throw std::runtime_error("unknown map size");
		}

		int left = 0;
		int top = 0;
		mf.getMapAttribute("west_bound", left);
		mf.getMapAttribute("north_bound", top);

		_originalBounds = MakeRectWH(vec2d{ (float)left, (float)top }, vec2d{ (float)width, (float)height }) * CELL_SIZE;

		bounds.left = std::min(bounds.left, left);
		bounds.top = std::min(bounds.top, top);
		bounds.right = std::max(bounds.right, width);
		bounds.bottom = std::max(bounds.bottom, height);

		_world.reset(new World(bounds));
		_world->Import(mf);
	}
	else
	{
		_world.reset(new World(bounds));
	}

}

EditorContext::~EditorContext()
{
}

void EditorContext::Step(float dt)
{
//	_world->Step(dt);
}
