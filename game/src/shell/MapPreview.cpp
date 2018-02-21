#include "MapPreview.h"
#include <gc/WorldCache.h>
#include <gc/World.h>
#include <ui/DataSource.h>
#include <ui/InputContext.h>
#include <ui/LayoutContext.h>
#include <ui/Rating.h>
#include <ui/StateContext.h>
#include <render/WorldView.h>
#include <video/RenderContext.h>
#include <video/TextureManager.h>

MapPreview::MapPreview(FS::FileSystem &fs, WorldView &worldView, WorldCache &mapCache)
	: _fs(fs)
	, _worldView(worldView)
	, _worldCache(mapCache)
{
}

void MapPreview::SetMapName(std::shared_ptr<UI::RenderData<std::string_view>> mapName)
{
	_mapName = std::move(mapName);
}

void MapPreview::SetRating(std::shared_ptr<UI::RenderData<unsigned int>> rating)
{
	if (rating)
	{
		_rating = std::make_shared<UI::Rating>();
		AddFront(_rating);
		_rating->SetRating(std::move(rating));
	}
	else if (_rating)
	{
		UnlinkChild(*_rating);
		_rating.reset();
	}
}

void MapPreview::Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const
{
	vec2d pxPadding = UI::ToPx(vec2d{ _padding, _padding }, lc);
	vec2d pxViewSize = lc.GetPixelSize() - pxPadding * 2;
	FRECT pxContentRect = MakeRectWH(pxPadding, pxViewSize);

	if (_mapName)
	{
		const World &world = _worldCache.GetCachedWorld(_fs, _mapName->GetRenderValue(dc, sc));

		vec2d worldSize = Size(world.GetBounds());
		vec2d eye = Center(world.GetBounds());
		float zoom = std::max(pxViewSize.x / worldSize.x, pxViewSize.y / worldSize.y);

		rc.PushClippingRect(FRectToRect(pxContentRect));
		rc.PushWorldTransform(ComputeWorldTransformOffset(pxContentRect, eye, zoom), zoom);
		_worldView.Render(rc, world);
		rc.PopTransform();
		rc.PopClippingRect();
	}

	if (_locked)
	{
		rc.DrawSprite(pxContentRect, _texLockShade.GetTextureId(texman), 0xffffffff, 0);

		vec2d pxSize = ToPx(_texLock.GetTextureSize(texman), lc);
		auto rect = MakeRectWH(Vec2dFloor((lc.GetPixelSize() - pxSize) / 2), pxSize);
		rc.DrawSprite(rect, _texLock.GetTextureId(texman), 0x88888888, 0);
	}
	else
	{
		FRECT sel = MakeRectRB(vec2d{}, lc.GetPixelSize());
		if (sc.GetState() == "Pushed")
		{
			rc.DrawSprite(sel, _texSelection.GetTextureId(texman), 0xffffffff, 0);
			rc.DrawBorder(sel, _texSelection.GetTextureId(texman), 0xffffffff, 0);
		}
		else if (sc.GetState() == "Focused")
		{
			rc.DrawBorder(sel, _texSelection.GetTextureId(texman), 0xffffffff, 0);
		}
		else if (sc.GetState() == "Hover")
		{
			rc.DrawSprite(sel, _texSelection.GetTextureId(texman), 0x44444444, 0);
		}
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
