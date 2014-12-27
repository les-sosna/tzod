#include "EditorContext.h"
#include "MapFile.h"
#include "gc/World.h"

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
	
	std::string theme;
	mf.getMapAttribute("theme", theme);
//	themeManager.ApplyTheme(themeManager.FindTheme(theme), tm);
	
	_world.reset(new World(width, height));
	_world->Import(mf);
}

EditorContext::EditorContext(float width, float height)
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
