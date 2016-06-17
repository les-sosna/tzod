#include "SinglePlayer.h"
#include <render/WorldView.h>
#include <MapFile.h>
#include <fs/FileSystem.h>
#include <gc/World.h>
#include <video/DrawingContext.h>

SinglePlayer::SinglePlayer(UI::LayoutManager &manager, TextureManager &texman, WorldView &worldView, FS::FileSystem &fs)
	: UI::Dialog(manager, texman, 600, 400)
	, _worldView(worldView)
{
	auto stream = fs.GetFileSystem("maps")->Open("dm5.map")->QueryStream();
	MapFile file(*stream, false);

	int width, height;
	if (!file.getMapAttribute("width", width) ||
		!file.getMapAttribute("height", height))
	{
		throw std::runtime_error("unknown map size");
	}

	_world.reset(new World(width, height));
	_world->Import(file);
}

void SinglePlayer::Draw(bool hovered, bool focused, bool enabled, vec2d size, UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	UI::Dialog::Draw(hovered, focused, enabled, size, ic, dc, texman);

	_worldView.Render(dc, *_world, { 0, 0, 256, 256}, vec2d(_world->_sx, _world->_sy) / 2, 1.f / 8, false, false, true);
	dc.SetMode(RM_INTERFACE);
}

