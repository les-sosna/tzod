#include "inc/ui/DataSource.h"
#include "inc/ui/Rating.h"
#include "inc/ui/LayoutContext.h"
#include <video/RenderContext.h>
#include <video/TextureManager.h>

using namespace UI;

void Rating::SetRating(std::shared_ptr<RenderData<unsigned int>> rating)
{
	_rating = std::move(rating);
}

void Rating::Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, float time, bool hovered) const
{
	auto &spriteInfo = texman.GetSpriteInfo(_texture.GetTextureId(texman));
	vec2d spriteSize = { spriteInfo.pxFrameWidth, spriteInfo.pxFrameHeight };

	vec2d scale = lc.GetPixelSize() / vec2d{ spriteSize.x * _maxRating, spriteSize.y };
	float minScale = std::min(scale.x, scale.y);

	vec2d pxItemSize = ToPx(spriteSize, minScale);

	unsigned int rating = _rating ? _rating->GetRenderValue(dc, sc) : 0;
	for (unsigned int i = 0; i < _maxRating; i++)
	{
		rc.DrawSprite(MakeRectWH(vec2d{ pxItemSize.x * (float)i, 0 }, pxItemSize), _texture.GetTextureId(texman), 0xffffffff, i >= rating);
	}
}

vec2d Rating::GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const
{
	auto &spriteInfo = texman.GetSpriteInfo(_texture.GetTextureId(texman));
	vec2d spriteSize = { spriteInfo.pxFrameWidth, spriteInfo.pxFrameHeight };
	return Vec2dMulX(ToPx(spriteSize, scale), (float) _maxRating);
}

