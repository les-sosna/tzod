#include "inc/ui/Rectangle.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/LayoutContext.h"
#include <video/TextureManager.h>
#include <video/DrawingContext.h>

using namespace UI;

Rectangle::Rectangle(LayoutManager &manager)
	: Window(manager)
	, _drawBorder(true)
	, _drawBackground(true)
{
}

float Rectangle::GetTextureWidth(TextureManager &texman) const
{
	return (-1 != _texture) ? texman.GetFrameWidth(_texture, _frame) : 1;
}

float Rectangle::GetTextureHeight(TextureManager &texman) const
{
	return (-1 != _texture) ? texman.GetFrameHeight(_texture, _frame) : 1;
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

unsigned int Rectangle::GetFrameCount() const
{
	return (-1 != _texture) ? GetManager().GetTextureManager().GetFrameCount(_texture) : 0;
}

void Rectangle::Draw(const LayoutContext &lc, InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	FRECT dst = { 0, 0, lc.GetPixelSize().x, lc.GetPixelSize().y };

	if (-1 != _texture)
	{
		if (_drawBackground)
		{
			float border = _drawBorder ? texman.GetBorderSize(_texture) : 0.f;
			FRECT client = { dst.left + border, dst.top + border, dst.right - border, dst.bottom - border };
			if (_textureStretchMode == StretchMode::Stretch)
			{
				dc.DrawSprite(client, _texture, _backColor, _frame);
			}
			else
			{
				RectRB clip;
				FRectToRect(&clip, &client);
				dc.PushClippingRect(clip);

				float frameWidth = texman.GetFrameWidth(_texture, _frame);
				float frameHeight = texman.GetFrameHeight(_texture, _frame);

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

				dc.DrawSprite(client, _texture, _backColor, _frame);

				dc.PopClippingRect();
			}
		}
		if (_drawBorder)
		{
			dc.DrawBorder(dst, _texture, _borderColor, _frame);
		}
	}
}

