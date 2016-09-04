#include "MapCache.h"
#include "MapPreview.h"
#include <render/WorldView.h>
#include <gc/World.h>
#include <ui/DataSource.h>
#include <ui/InputContext.h>
#include <ui/LayoutContext.h>
#include <ui/Rating.h>
#include <ui/StateContext.h>
#include <video/DrawingContext.h>
#include <video/TextureManager.h>

MapPreview::MapPreview(UI::LayoutManager &manager, TextureManager &texman, FS::FileSystem &fs, WorldView &worldView, MapCache &mapCache)
	: UI::Window(manager)
	, _fs(fs)
	, _worldView(worldView)
	, _mapCache(mapCache)
	, _font(texman.FindSprite("font_default"))
	, _texSelection(texman.FindSprite("ui/selection"))
	, _rating(std::make_shared<UI::Rating>(manager, texman))
{
	AddFront(_rating);
}

void MapPreview::SetMapName(std::shared_ptr<UI::DataSource<const std::string&>> mapName)
{
	_mapName = std::move(mapName);
}

void MapPreview::SetRating(std::shared_ptr<UI::DataSource<unsigned int>> rating)
{
	_rating->SetRating(std::move(rating));
}

void MapPreview::Draw(const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	if (_mapName)
	{
		const World &world = _mapCache.GetCachedWorld(_fs, _mapName->GetValue(sc));

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

		dc.DrawBitmapText(Vec2dFloor(lc.GetPixelSize() / 2), lc.GetScale(), _font, 0xffffffff, _mapName->GetValue(sc), alignTextCC);
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

FRECT MapPreview::GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::StateContext &sc, const UI::Window &child) const
{
	if (_rating.get() == &child)
	{
		vec2d pxPadding = UI::ToPx(vec2d{ _padding, _padding }, lc);
		return MakeRectWH(pxPadding, lc.GetPixelSize() / 2);
	}
	return UI::Window::GetChildRect(texman, lc, sc, child);
}

