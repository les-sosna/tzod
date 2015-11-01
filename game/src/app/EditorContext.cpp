#include "inc/app/EditorContext.h"
#include <gc/World.h>
#include <MapFile.h>

EditorContext::EditorContext(FS::Stream &stream)
{
	MapFile mf(stream, false);

	int width = 0;
	int height = 0;
	if (!mf.getMapAttribute("width", width) ||
		!mf.getMapAttribute("height", height))
	{
		throw std::runtime_error("unknown map size");
	}

	_world.reset(new World(width, height));
	_world->Import(mf);
}

EditorContext::EditorContext(int width, int height)
	: _world(new World(width, height))
{
}

EditorContext::~EditorContext()
{
}

void EditorContext::Step(float dt)
{
	_world->Step(dt);
}
