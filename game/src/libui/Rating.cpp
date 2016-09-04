#include "inc/ui/Rating.h"
#include "inc/ui/LayoutContext.h"
#include <video/DrawingContext.h>
#include <video/TextureManager.h>

using namespace UI;

Rating::Rating(LayoutManager &manager, TextureManager &texman)
	: Window(manager)
	, _texture(texman.FindSprite("ui/star"))
{
}

void Rating::Draw(const StateContext &sc, const LayoutContext &lc, const InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	auto &spriteInfo = texman.GetSpriteInfo(_texture);
	vec2d spriteSize = { spriteInfo.pxFrameWidth, spriteInfo.pxFrameHeight };

	vec2d scale = lc.GetPixelSize() / vec2d{ spriteSize.x * _maxRating, spriteSize.y };
	float minScale = std::min(scale.x, scale.y);

	vec2d pxItemSize = ToPx(spriteSize, minScale);

	for (unsigned int i = 0; i < _maxRating; i++)
	{
		dc.DrawSprite(MakeRectWH(vec2d{ pxItemSize.x * (float)i, 0 }, pxItemSize), _texture, 0xffffffff, i >= _rating);
	}
}

vec2d Rating::GetContentSize(TextureManager &texman, const StateContext &sc, float scale) const
{
	auto &spriteInfo = texman.GetSpriteInfo(_texture);
	vec2d spriteSize = { spriteInfo.pxFrameWidth, spriteInfo.pxFrameHeight };
	return Vec2dMulX(spriteSize, (float) _maxRating);
}

