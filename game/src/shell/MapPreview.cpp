#include "MapPreview.h"
#include <render/WorldView.h>
#include <MapFile.h>
#include <fs/FileSystem.h>
#include <gc/World.h>
#include <ui/InputContext.h>
#include <video/DrawingContext.h>
#include <video/TextureManager.h>
#include <thread>

MapPreview::MapPreview(UI::LayoutManager &manager, TextureManager &texman, WorldView &worldView)
	: UI::Window(manager)
	, _worldView(worldView)
	, _font(texman.FindSprite("font_small"))
{
}

void MapPreview::SetMapName(std::string mapName, FS::FileSystem &fs)
{
	try
	{
		auto stream = fs.GetFileSystem("maps")->Open(mapName + ".map")->QueryStream();

		MapFile file(*stream, false);

		int width, height;
		if (!file.getMapAttribute("width", width) ||
			!file.getMapAttribute("height", height))
		{
			throw std::runtime_error("unknown map size");
		}

		_world.reset(new World(width, height));
		_world->Import(file);

		_title = mapName;
	}
	catch(const std::exception &e)
	{
		_title = e.what();
		_world.reset();
	}
}

void MapPreview::Draw(bool hovered, bool focused, bool enabled, vec2d size, UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	UI::Window::Draw(hovered, focused, enabled, size, ic, dc, texman);

	if (_world)
	{
		vec2d worldSize(_world->_sx, _world->_sy);
		vec2d eye = worldSize / 2;
		float zoom = std::max(size.x / _world->_sx, size.y / _world->_sy);

		if (hovered)
		{
			zoom *= 1.1f;
			eye += worldSize * (ic.GetMousePos() - size / 2) / size / 10;
		}

		_worldView.Render(
			dc,
			*_world,
			{ 0, 0, (int)size.x, (int)size.y }, // viewport
			eye,
			zoom,
			false, // editorMode
			false, // drawGrid
			false  // nightMode
		);
		dc.SetMode(RM_INTERFACE);
	}
	dc.DrawBitmapText(0, 0, _font, 0xffffffff, _title);
}

