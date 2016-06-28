#include "inc/ui/Button.h"
#include "inc/ui/InputContext.h"
#include "inc/ui/LayoutContext.h"
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

void ButtonBase::OnPointerMove(InputContext &ic, vec2d size, vec2d pointerPosition, PointerType pointerType, unsigned int pointerID, bool captured)
{
	if( eventMouseMove )
		eventMouseMove(pointerPosition.x, pointerPosition.y);
}

bool ButtonBase::OnPointerDown(InputContext &ic, vec2d size, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID)
{
	if( !ic.HasCapturedPointers(this) && 1 == button ) // primary button only
	{
		if( eventMouseDown )
			eventMouseDown(pointerPosition.x, pointerPosition.y);
		return true;
	}
	return false;
}

void ButtonBase::OnPointerUp(InputContext &ic, vec2d size, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID)
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

void ButtonBase::OnTap(InputContext &ic, vec2d size, vec2d pointerPosition)
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
// button class implementation

std::shared_ptr<Button> Button::Create(Window *parent, TextureManager &texman, const std::string &text, float x, float y, float w, float h)
{
	auto res = std::make_shared<Button>(parent->GetManager(), texman);
	res->Move(x, y);
	res->SetText(texman, text);
	if( w >= 0 && h >= 0 )
	{
		res->Resize(w, h);
	}

	parent->AddFront(res);

	return res;
}

Button::Button(LayoutManager &manager, TextureManager &texman)
  : ButtonBase(manager)
  , _font((size_t)-1)
  , _icon((size_t)-1)
{
	SetTexture(texman, "ui/button", true);
	SetFont(texman, "font_default");
}

void Button::SetFont(TextureManager &texman, const char *fontName)
{
	_font = texman.FindSprite(fontName);
}

void Button::SetIcon(TextureManager &texman, const char *spriteName)
{
	_icon = spriteName ? texman.FindSprite(spriteName) : (size_t)-1;
}

void Button::Draw(const LayoutContext &lc, InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	State state = GetState(lc, ic);

	const_cast<Button*>(this)->SetFrame(state);

	ButtonBase::Draw(lc, ic, dc, texman);

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

	if (_icon != -1)
	{
		float iconHeight = texman.GetFrameHeight(_icon, 0);
		float textHeight = texman.GetFrameHeight(_font, 0);

		float x = lc.GetPixelSize().x / 2;
		float y = (lc.GetPixelSize().y - iconHeight - textHeight) / 2 + iconHeight;

		dc.DrawSprite(_icon, 0, c, x, y - iconHeight / 2, { 1, 0 });
		dc.DrawBitmapText(x, y, _font, c, GetText(), alignTextCT);
	}
	else
	{
		vec2d pos = lc.GetPixelSize() / 2;
		dc.DrawBitmapText(pos.x, pos.y, _font, c, GetText(), alignTextCC);
	}
}


///////////////////////////////////////////////////////////////////////////////
// TextButton

TextButton::TextButton(LayoutManager &manager, TextureManager &texman)
  : ButtonBase(manager)
  , _fontTexture((size_t) -1)
  , _drawShadow(true)
{
	SetTexture(texman, nullptr, false);
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

void TextButton::SetDrawShadow(bool drawShadow)
{
	_drawShadow = drawShadow;
}

bool TextButton::GetDrawShadow() const
{
	return _drawShadow;
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
	if( _drawShadow && stateDisabled != state )
	{
		dc.DrawBitmapText(1, 1, _fontTexture, 0xff000000, GetText());
	}
	dc.DrawBitmapText(0, 0, _fontTexture, colors[state], GetText());
}

///////////////////////////////////////////////////////////////////////////////

ImageButton::ImageButton(LayoutManager &manager)
  : ButtonBase(manager)
{
}

void ImageButton::Draw(const LayoutContext &lc, InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	State state = GetState(lc, ic);

	const_cast<ImageButton*>(this)->SetFrame(state);

	ButtonBase::Draw(lc, ic, dc, texman);
}

///////////////////////////////////////////////////////////////////////////////

CheckBox::CheckBox(LayoutManager &manager, TextureManager &texman)
  : ButtonBase(manager)
  , _fontTexture(texman.FindSprite("font_small"))
  , _boxTexture(texman.FindSprite("ui/checkbox"))
  , _drawShadow(true)
  , _isChecked(false)
{
	SetTexture(texman, nullptr, false);
	AlignSizeToContent(texman);
}

void CheckBox::AlignSizeToContent(TextureManager &texman)
{
	float th = texman.GetFrameHeight(_fontTexture, 0);
	float tw = texman.GetFrameWidth(_fontTexture, 0);
	float bh = texman.GetFrameHeight(_boxTexture, GetFrame());
	float bw = texman.GetFrameWidth(_boxTexture, GetFrame());
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
	State state = GetState(lc, ic);
	const_cast<CheckBox*>(this)->SetFrame(_isChecked ? state + 4 : state);

	ButtonBase::Draw(lc, ic, dc, texman);

	float bh = texman.GetFrameHeight(_boxTexture, GetFrame());
	float bw = texman.GetFrameWidth(_boxTexture, GetFrame());
	float th = texman.GetFrameHeight(_fontTexture, 0);

	FRECT box = {0, (lc.GetPixelSize().y - bh) / 2, bw, (lc.GetPixelSize().y - bh) / 2 + bh};
	dc.DrawSprite(box, _boxTexture, GetBackColor(), GetFrame());

	// grep 'enum State'
	SpriteColor colors[] =
	{
		SpriteColor(0xffffffff), // Normal
		SpriteColor(0xffffffff), // Hottrack
		SpriteColor(0xffffffff), // Pushed
		SpriteColor(0xffffffff), // Disabled
	};
	if( _drawShadow && stateDisabled != state)
	{
		dc.DrawBitmapText(bw + 1, (lc.GetPixelSize().y - th) / 2 + 1, _fontTexture, 0xff000000, GetText());
	}
	dc.DrawBitmapText(bw, (lc.GetPixelSize().y - th) / 2, _fontTexture, colors[state], GetText());
}
