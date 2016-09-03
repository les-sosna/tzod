#include "MapCache.h"
#include "MapPreview.h"
#include <render/WorldView.h>
#include <gc/World.h>
#include <ui/DataSource.h>
#include <ui/InputContext.h>
#include <ui/LayoutContext.h>
#include <ui/StateContext.h>
#include <video/DrawingContext.h>
#include <video/TextureManager.h>
#include <thread>

MapPreview::MapPreview(UI::LayoutManager &manager, TextureManager &texman, FS::FileSystem &fs, WorldView &worldView, MapCache &mapCache)
	: UI::Window(manager)
	, _fs(fs)
	, _worldView(worldView)
	, _mapCache(mapCache)
	, _font(texman.FindSprite("font_default"))
	, _texSelection(texman.FindSprite("ui/selection"))
{
}

void MapPreview::SetMapName(std::shared_ptr<UI::TextSource> mapName)
{
	_mapName = std::move(mapName);
}

void MapPreview::Draw(const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	if (_mapName)
	{
		const World &world = _mapCache.GetCachedWorld(_fs, _mapName->GetText(sc));

		vec2d pxPadding = UI::ToPx(vec2d{ _padding, _padding }, lc);
		vec2d pxViewSize = lc.GetPixelSize() - pxPadding * 2;

		vec2d worldSize = Size(world._bounds);
		vec2d eye = Center(world._bounds);
		float zoom = std::max(pxViewSize.x / worldSize.x, pxViewSize.y / worldSize.y);

		_worldView.Render(
			dc,
			world,
			FRectToRect(MakeRectWH(pxPadding, pxViewSize)),
			eye,
			zoom,
			false, // editorMode
			false, // drawGrid
			false  // nightMode
		);

		dc.DrawBitmapText(Vec2dFloor(lc.GetPixelSize() / 2), lc.GetScale(), _font, 0xffffffff, _mapName->GetText(sc), alignTextCC);
	}

	FRECT sel = MakeRectRB(vec2d{}, lc.GetPixelSize());
	if (sc.GetState() == "Focused")
	{
		dc.DrawSprite(sel, _texSelection, 0xffffffff, 0);
		dc.DrawBorder(sel, _texSelection, 0xffffffff, 0);
	}
	else if (sc.GetState() == "Unfocused")
	{
		dc.DrawBorder(sel, _texSelection, 0xffffffff, 0);
	}
	else if (sc.GetState() == "Hover")
	{
		dc.DrawSprite(sel, _texSelection, 0x44444444, 0);
	}
}

