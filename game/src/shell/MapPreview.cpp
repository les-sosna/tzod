#include "MapPreview.h"
#include <render/WorldView.h>
#include <MapFile.h>
#include <fs/FileSystem.h>
#include <gc/World.h>
#include <ui/InputContext.h>
#include <ui/LayoutContext.h>
#include <video/DrawingContext.h>
#include <video/TextureManager.h>
#include <thread>

MapPreview::MapPreview(UI::LayoutManager &manager, TextureManager &texman, WorldView &worldView)
	: UI::ButtonBase(manager)
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

		int left = 0;
		int top = 0;
		file.getMapAttribute("west_bound", left);
		file.getMapAttribute("north_bound", top);

		_world.reset(new World(RectRB{ left, top, left + width, top + height }));
		_world->Import(file);

		_title = mapName;
	}
	catch(const std::exception &e)
	{
		_title = e.what();
		_world.reset();
	}
}

void MapPreview::Draw(const UI::LayoutContext &lc, UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	UI::ButtonBase::Draw(lc, ic, dc, texman);

	if (_world)
	{
		State state = GetState(lc, ic);

		vec2d worldSize = Size(_world->_bounds);
		vec2d eye = Center(_world->_bounds);
		float zoom = std::max(lc.GetPixelSize().x / worldSize.x, lc.GetPixelSize().y / worldSize.y);

		if (state == statePushed)
		{
			zoom *= 1.1f;
			eye += worldSize * (ic.GetMousePos() - lc.GetPixelSize() / 2) / lc.GetPixelSize() / 10;
		}

		_worldView.Render(
			dc,
			*_world,
			{ 0, 0, (int)lc.GetPixelSize().x, (int)lc.GetPixelSize().y }, // viewport
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

