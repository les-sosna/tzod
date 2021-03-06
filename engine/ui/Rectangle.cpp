#include "inc/ui/Rectangle.h"
#include "inc/ui/DataSource.h"
#include "inc/ui/LayoutContext.h"
#include <video/TextureManager.h>
#include <video/RenderContext.h>

using namespace UI;

Rectangle::Rectangle()
	: _backColor(std::make_shared<StaticValue<SpriteColor>>(0xffffffff))
	, _borderColor(std::make_shared<StaticValue<SpriteColor>>(0xffffffff))
	, _drawBorder(true)
	, _drawBackground(true)
{
}

void Rectangle::SetBackColor(std::shared_ptr<RenderData<SpriteColor>> color)
{
	_backColor = std::move(color);
}

void Rectangle::SetBorderColor(std::shared_ptr<RenderData<SpriteColor>> color)
{
	_borderColor = std::move(color);
}

float Rectangle::GetTextureWidth(TextureManager &texman) const
{
	return _texture.Empty() ? 1 : texman.GetFrameWidth(_texture.GetTextureId(texman), 0);
}

float Rectangle::GetTextureHeight(TextureManager &texman) const
{
	return _texture.Empty() ? 1 : texman.GetFrameHeight(_texture.GetTextureId(texman), 0);
}

void Rectangle::SetTexture(Texture texture)
{
	_texture = std::move(texture);
}

void Rectangle::SetTextureStretchMode(StretchMode stretchMode)
{
	_textureStretchMode = stretchMode;
}

void Rectangle::Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, const Plat::Input &input, float time, bool hovered) const
{
	if (!_texture.Empty() && (_drawBackground || _drawBorder))
	{
		FRECT dst = MakeRectWH(lc.GetPixelSize());
		unsigned int frame = _frame ? _frame->GetRenderValue(dc, sc) : 0;
		size_t texture = _texture.GetTextureId(texman);

		if (_drawBackground)
		{
			float border = _drawBorder ? texman.GetBorderSize(texture) : 0.f;
			FRECT client = { dst.left + border, dst.top + border, dst.right - border, dst.bottom - border };
			if (_textureStretchMode == StretchMode::Stretch)
			{
				rc.DrawSprite(client, texture, _backColor->GetRenderValue(dc, sc), frame);
			}
			else
			{
				RectRB clip = FRectToRect(client);
				rc.PushClippingRect(clip);

				float frameWidth = texman.GetFrameWidth(texture, frame);
				float frameHeight = texman.GetFrameHeight(texture, frame);

				if (WIDTH(client) * frameHeight > HEIGHT(client) * frameWidth)
				{
					float newHeight = WIDTH(client) / frameWidth * frameHeight;
					client.top = (HEIGHT(client) - newHeight) / 2;
					client.bottom = client.top + newHeight;
				}
				else
				{
					float newWidth = HEIGHT(client) / frameHeight * frameWidth;
					client.left = (WIDTH(client) - newWidth) / 2;
					client.right = client.left + newWidth;
				}

				rc.DrawSprite(client, texture, _backColor->GetRenderValue(dc, sc), frame);

				rc.PopClippingRect();
			}
		}

		if (_drawBorder)
		{
			rc.DrawBorder(dst, texture, _borderColor->GetRenderValue(dc, sc), frame);
		}
	}
}

