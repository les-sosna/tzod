#include "inc/ui/Rectangle.h"
#include "inc/ui/DataSource.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/LayoutContext.h"
#include <video/TextureManager.h>
#include <video/DrawingContext.h>

using namespace UI;

Rectangle::Rectangle(LayoutManager &manager)
	: Window(manager)
	, _backColor(std::make_shared<StaticValue<SpriteColor>>(0xffffffff))
	, _borderColor(std::make_shared<StaticValue<SpriteColor>>(0xffffffff))
	, _drawBorder(true)
	, _drawBackground(true)
{
}

void Rectangle::SetBackColor(std::shared_ptr<DataSource<SpriteColor>> color)
{
	_backColor = std::move(color);
}

void Rectangle::SetBorderColor(std::shared_ptr<DataSource<SpriteColor>> color)
{
	_borderColor = std::move(color);
}

float Rectangle::GetTextureWidth(TextureManager &texman) const
{
	return (-1 != _texture) ? texman.GetFrameWidth(_texture, 0) : 1;
}

float Rectangle::GetTextureHeight(TextureManager &texman) const
{
	return (-1 != _texture) ? texman.GetFrameHeight(_texture, 0) : 1;
}

void Rectangle::SetTexture(TextureManager &texman, const char *tex, bool fitSize)
{
	if (tex)
	{
		_texture = texman.FindSprite(tex);
		if (fitSize)
		{
			Resize(GetTextureWidth(texman), GetTextureHeight(texman));
		}
	}
	else
	{
		_texture = (size_t)-1;
	}
}

void Rectangle::SetTextureStretchMode(StretchMode stretchMode)
{
	_textureStretchMode = stretchMode;
}

void Rectangle::Draw(const StateContext &sc, const LayoutContext &lc, const InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	if (-1 != _texture)
	{
		FRECT dst = MakeRectWH(lc.GetPixelSize());
		unsigned int frame = _frame ? _frame->GetValue(sc) : 0;

		if (_drawBackground)
		{
			float border = _drawBorder ? texman.GetBorderSize(_texture) : 0.f;
			FRECT client = { dst.left + border, dst.top + border, dst.right - border, dst.bottom - border };
			if (_textureStretchMode == StretchMode::Stretch)
			{
				dc.DrawSprite(client, _texture, _backColor->GetValue(sc), frame);
			}
			else
			{
				RectRB clip = FRectToRect(client);
				dc.PushClippingRect(clip);

				float frameWidth = texman.GetFrameWidth(_texture, frame);
				float frameHeight = texman.GetFrameHeight(_texture, frame);

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

				dc.DrawSprite(client, _texture, _backColor->GetValue(sc), frame);

				dc.PopClippingRect();
			}
		}
		if (_drawBorder)
		{
			dc.DrawBorder(dst, _texture, _borderColor->GetValue(sc), frame);
		}
	}
}

