#include "MapPreview.h"
#include <gc/MapCache.h>
#include <gc/World.h>
#include <ui/DataSource.h>
#include <ui/InputContext.h>
#include <ui/LayoutContext.h>
#include <ui/Rating.h>
#include <ui/StateContext.h>
#include <render/WorldView.h>
#include <video/RenderContext.h>
#include <video/TextureManager.h>

MapPreview::MapPreview(FS::FileSystem &fs, WorldView &worldView, MapCache &mapCache)
	: _fs(fs)
	, _worldView(worldView)
	, _mapCache(mapCache)
	, _rating(std::make_shared<UI::Rating>())
{
	AddFront(_rating);
}

void MapPreview::SetMapName(std::shared_ptr<UI::RenderData<const std::string&>> mapName)
{
	_mapName = std::move(mapName);
}

void MapPreview::SetRating(std::shared_ptr<UI::RenderData<unsigned int>> rating)
{
	_rating->SetRating(std::move(rating));
}

void MapPreview::Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const
{
	vec2d pxPadding = UI::ToPx(vec2d{ _padding, _padding }, lc);
	vec2d pxViewSize = lc.GetPixelSize() - pxPadding * 2;
	FRECT pxContentRect = MakeRectWH(pxPadding, pxViewSize);

	if (_mapName)
	{
		const World &world = _mapCache.GetCachedWorld(_fs, _mapName->GetValue(dc, sc));

		vec2d worldSize = Size(world._bounds);
		vec2d eye = Center(world._bounds);
		float zoom = std::max(pxViewSize.x / worldSize.x, pxViewSize.y / worldSize.y);

		_worldView.Render(
			rc,
			world,
			FRectToRect(pxContentRect),
			eye,
			zoom,
			false, // editorMode
			false, // drawGrid
			false  // nightMode
		);
	}

	FRECT sel = MakeRectRB(vec2d{}, lc.GetPixelSize());
	if (sc.GetState() == "Pushed")
	{
		rc.DrawSprite(sel, _texSelection.GetTextureId(texman), 0xffffffff, 0);
		rc.DrawBorder(sel, _texSelection.GetTextureId(texman), 0xffffffff, 0);
	}
	else if (sc.GetState() == "Unfocused")
	{
		rc.DrawBorder(sel, _texSelection.GetTextureId(texman), 0xffffffff, 0);
	}
	else if (sc.GetState() == "Hover")
	{
		rc.DrawSprite(sel, _texSelection.GetTextureId(texman), 0x44444444, 0);
	}
	else if (sc.GetState() == "Disabled")
	{
		rc.DrawSprite(pxContentRect, _texLockShade.GetTextureId(texman), 0xffffffff, 0);

		vec2d pxSize = ToPx(_texLock.GetTextureSize(texman), lc);
		auto rect = MakeRectWH(Vec2dFloor((lc.GetPixelSize() - pxSize) / 2), pxSize);
		rc.DrawSprite(rect, _texLock.GetTextureId(texman), 0x88888888, 0);
	}
}

FRECT MapPreview::GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const
{
	if (_rating.get() == &child)
	{
		vec2d pxPadding = UI::ToPx(vec2d{ _padding, _padding }, lc);
		return MakeRectWH(pxPadding, lc.GetPixelSize() / 2);
	}
	return UI::Window::GetChildRect(texman, lc, dc, child);
}
