#include "inc/ui/Button.h"
#include "inc/ui/InputContext.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/Rectangle.h"
#include "inc/ui/Text.h"
#include "inc/ui/UIInput.h"
#include "inc/ui/GuiManager.h"
#include <video/TextureManager.h>
#include <video/DrawingContext.h>
#include <algorithm>

using namespace UI;

ButtonBase::ButtonBase(LayoutManager &manager)
  : Window(manager)
{
}

ButtonBase::State ButtonBase::GetState(const LayoutContext &lc, const InputContext &ic) const
{
	if (!lc.GetEnabled())
		return stateDisabled;

	vec2d pointerPosition = ic.GetMousePos();
	bool pointerInside = pointerPosition.x >= 0 && pointerPosition.y >= 0 && pointerPosition.x < lc.GetPixelSize().x && pointerPosition.y < lc.GetPixelSize().y;
	bool pointerPressed = ic.GetInput().IsMousePressed(1);

	if (pointerInside && pointerPressed && ic.HasCapturedPointers(this))
		return statePushed;

	if (ic.GetHovered())
		return stateHottrack;

	return stateNormal;
}

void ButtonBase::OnPointerMove(InputContext &ic, vec2d size, float scale, vec2d pointerPosition, PointerType pointerType, unsigned int pointerID, bool captured)
{
	if( eventMouseMove )
		eventMouseMove(pointerPosition.x, pointerPosition.y);
}

bool ButtonBase::OnPointerDown(InputContext &ic, vec2d size, float scale, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID)
{
	if( !ic.HasCapturedPointers(this) && 1 == button ) // primary button only
	{
		if( eventMouseDown )
			eventMouseDown(pointerPosition.x, pointerPosition.y);
		return true;
	}
	return false;
}

void ButtonBase::OnPointerUp(InputContext &ic, vec2d size, float scale, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID)
{
	bool pointerInside = pointerPosition.x < size.x && pointerPosition.y < size.y && pointerPosition.x >= 0 && pointerPosition.y >= 0;
	if( eventMouseUp )
		eventMouseUp(pointerPosition.x, pointerPosition.y);
	if(pointerInside)
	{
		OnClick();
		if( eventClick )
			eventClick();
	}
}

void ButtonBase::OnTap(InputContext &ic, vec2d size, float scale, vec2d pointerPosition)
{
	if( !ic.HasCapturedPointers(this))
	{
		OnClick();
		if( eventClick )
			eventClick();
	}
}

void ButtonBase::OnClick()
{
}

///////////////////////////////////////////////////////////////////////////////

Button::Button(LayoutManager &manager, TextureManager &texman)
	: ButtonBase(manager)
	, _background(std::make_shared<Rectangle>(manager))
	, _text(std::make_shared<Text>(manager, texman))
{
	AddFront(_background);
	AddFront(_text);

	_text->SetAlign(alignTextCC);

	SetFont(texman, "font_default");
	SetBackground(texman, "ui/button", true);
}

void Button::SetFont(TextureManager &texman, const char *fontName)
{
	_text->SetFont(texman, fontName);
}

void Button::SetIcon(LayoutManager &manager, TextureManager &texman, const char *spriteName)
{
	if (spriteName)
	{
		if (!_icon)
		{
			_icon = std::make_shared<Rectangle>(manager);
			AddFront(_icon);
		}
		_icon->SetTexture(texman, spriteName, true);
	}
	else
	{
		if (_icon)
		{
			UnlinkChild(*_icon);
			_icon.reset();
		}
	}
}

void Button::SetBackground(TextureManager &texman, const char *tex, bool fitSize)
{
	_background->SetTexture(texman, tex, fitSize);
	if (fitSize)
	{
		Resize(_background->GetWidth(), _background->GetHeight());
	}
}

FRECT Button::GetChildRect(vec2d size, float scale, const Window &child) const
{
	if (_background.get() == &child)
	{
		return MakeRectRB(vec2d{}, size);
	}

	if (_icon)
	{
		if (_text.get() == &child)
		{
			vec2d pxChildPos = Vec2dFloor(size / 2);
			pxChildPos.y += std::floor(_icon->GetHeight() * scale / 2);
			return MakeRectWH(pxChildPos, vec2d{});
		}

		if (_icon.get() == &child)
		{
			vec2d pxChildSize = Vec2dFloor(_icon->GetSize() * scale);
			vec2d pxChildPos = Vec2dFloor((size - pxChildSize) / 2);
			pxChildPos.y -= std::floor(_text->GetHeight() * scale / 2);
			return MakeRectWH(pxChildPos, pxChildSize);
		}
	}
	else
	{
		if (_text.get() == &child)
		{
			return MakeRectWH(Vec2dFloor(size / 2), vec2d{});
		}
	}

	return Window::GetChildRect(size, scale, child);
}

void Button::OnTextChange(TextureManager &texman)
{
	_text->SetText(texman, GetText());
}

void Button::Draw(const LayoutContext &lc, InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	ButtonBase::Draw(lc, ic, dc, texman);

	State state = GetState(lc, ic);

	SpriteColor c = 0;

	switch( state )
	{
	case statePushed:
		c = 0xffffffff;
		break;
	case stateHottrack:
		c = 0xffffffff;
		break;
	case stateNormal:
		c = 0xffffffff;
		break;
	case stateDisabled:
		c = 0xbbbbbbbb;
		break;
	default:
		assert(false);
	}

	_background->SetFrame(state);
	_text->SetFontColor(c);

	if (_icon)
	{
		_icon->SetBackColor(c);
		_icon->SetBorderColor(c);
	}
}


///////////////////////////////////////////////////////////////////////////////
// TextButton

TextButton::TextButton(LayoutManager &manager, TextureManager &texman)
  : ButtonBase(manager)
  , _fontTexture((size_t) -1)
{
}

void TextButton::AlignSizeToContent(TextureManager &texman)
{
	if( -1 != _fontTexture )
	{
		float w = texman.GetFrameWidth(_fontTexture, 0);
		float h = texman.GetFrameHeight(_fontTexture, 0);
		Resize((w - 1) * (float) GetText().length(), h + 1);
	}
}

void TextButton::SetFont(TextureManager &texman, const char *fontName)
{
	_fontTexture = texman.FindSprite(fontName);
	AlignSizeToContent(texman);
}

void TextButton::OnTextChange(TextureManager &texman)
{
	AlignSizeToContent(texman);
}

void TextButton::Draw(const LayoutContext &lc, InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	ButtonBase::Draw(lc, ic, dc, texman);

	// grep 'enum State'
	SpriteColor colors[] =
	{
		SpriteColor(0xffffffff), // normal
		SpriteColor(0xffccccff), // hottrack
		SpriteColor(0xffccccff), // pushed
		SpriteColor(0xAAAAAAAA), // disabled
	};
	State state = GetState(lc, ic);
	dc.DrawBitmapText(0, 0, _fontTexture, colors[state], GetText());
}

///////////////////////////////////////////////////////////////////////////////

CheckBox::CheckBox(LayoutManager &manager, TextureManager &texman)
  : ButtonBase(manager)
  , _fontTexture(texman.FindSprite("font_small"))
  , _boxTexture(texman.FindSprite("ui/checkbox"))
  , _isChecked(false)
{
	AlignSizeToContent(texman);
}

void CheckBox::AlignSizeToContent(TextureManager &texman)
{
	float th = texman.GetFrameHeight(_fontTexture, 0);
	float tw = texman.GetFrameWidth(_fontTexture, 0);
	float bh = texman.GetFrameHeight(_boxTexture, 0);
	float bw = texman.GetFrameWidth(_boxTexture, 0);
	Resize(bw + (tw - 1) * (float) GetText().length(), std::max(th + 1, bh));
}

void CheckBox::SetCheck(bool checked)
{
	_isChecked = checked;
}

void CheckBox::OnClick()
{
	SetCheck(!GetCheck());
}

void CheckBox::OnTextChange(TextureManager &texman)
{
	AlignSizeToContent(texman);
}

void CheckBox::Draw(const LayoutContext &lc, InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	ButtonBase::Draw(lc, ic, dc, texman);

	State state = GetState(lc, ic);
	size_t frame = _isChecked ? state + 4 : state;

	float bh = texman.GetFrameHeight(_boxTexture, frame);
	float bw = texman.GetFrameWidth(_boxTexture, frame);
	float th = texman.GetFrameHeight(_fontTexture, 0);

	FRECT box = {0, (lc.GetPixelSize().y - bh) / 2, bw, (lc.GetPixelSize().y - bh) / 2 + bh};
	dc.DrawSprite(box, _boxTexture, 0xffffffff, frame);

	// grep 'enum State'
	SpriteColor colors[] =
	{
		SpriteColor(0xffffffff), // Normal
		SpriteColor(0xffffffff), // Hottrack
		SpriteColor(0xffffffff), // Pushed
		SpriteColor(0xffffffff), // Disabled
	};
	dc.DrawBitmapText(bw, (lc.GetPixelSize().y - th) / 2, _fontTexture, colors[state], GetText());
}
