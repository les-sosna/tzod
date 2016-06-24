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

void MapPreview::Draw(const UI::LayoutContext &lc, UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	UI::ButtonBase::Draw(lc, ic, dc, texman);

	if (_world)
	{
		State state = GetState(lc, ic);

		vec2d worldSize(_world->_sx, _world->_sy);
		vec2d eye = worldSize / 2;
		float zoom = std::max(lc.GetSize().x / _world->_sx, lc.GetSize().y / _world->_sy);

		if (state == statePushed)
		{
			zoom *= 1.1f;
			eye += worldSize * (ic.GetMousePos() - lc.GetSize() / 2) / lc.GetSize() / 10;
		}

		_worldView.Render(
			dc,
			*_world,
			{ 0, 0, (int)lc.GetSize().x, (int)lc.GetSize().y }, // viewport
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

