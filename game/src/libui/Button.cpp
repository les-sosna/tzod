#include "inc/ui/Button.h"
#include "inc/ui/DataSource.h"
#include "inc/ui/InputContext.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/Rectangle.h"
#include "inc/ui/StateContext.h"
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

void ButtonBase::OnPointerMove(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, PointerType pointerType, unsigned int pointerID, bool captured)
{
	if( eventMouseMove )
		eventMouseMove(pointerPosition.x, pointerPosition.y);
}

bool ButtonBase::OnPointerDown(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID)
{
	if( !ic.HasCapturedPointers(this) && 1 == button ) // primary button only
	{
		if( eventMouseDown )
			eventMouseDown(pointerPosition.x, pointerPosition.y);
		return true;
	}
	return false;
}

void ButtonBase::OnPointerUp(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID)
{
	auto size = lc.GetPixelSize();
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

void ButtonBase::OnTap(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition)
{
	if( !ic.HasCapturedPointers(this))
	{
		OnClick();
		if( eventClick )
			eventClick();
	}
}

void ButtonBase::PushState(StateContext &sc, const LayoutContext &lc, const InputContext &ic) const
{
	switch (GetState(lc, ic))
	{
	case statePushed:
		sc.SetState("Pushed");
		break;
	case stateHottrack:
		sc.SetState("Hover");
		break;
	case stateNormal:
		sc.SetState("Idle");
		break;
	case stateDisabled:
		sc.SetState("Disabled");
		break;
	default:
		assert(false);
	}
}

void ButtonBase::OnClick()
{
}

///////////////////////////////////////////////////////////////////////////////

static const auto c_textColor = std::make_shared<ColorMap>(0xffffffff, // default
	ColorMap::ColorMapType{ { "Disabled", 0xbbbbbbbb }, { "Hover", 0xffccccff } });

Button::Button(LayoutManager &manager, TextureManager &texman)
	: ButtonBase(manager)
	, _background(std::make_shared<Rectangle>(manager))
	, _text(std::make_shared<Text>(manager, texman))
{
	AddFront(_background);
	AddFront(_text);

	_text->SetAlign(alignTextCC);
	_text->SetFontColor(c_textColor);

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
			_icon->SetBackColor(c_textColor);
			_icon->SetBorderColor(c_textColor);
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

void Button::SetText(std::shared_ptr<TextSource> text)
{
	_text->SetText(std::move(text));
}

void Button::SetBackground(TextureManager &texman, const char *tex, bool fitSize)
{
	_background->SetTexture(texman, tex, fitSize);
	if (fitSize)
	{
		Resize(_background->GetWidth(), _background->GetHeight());
	}
}

FRECT Button::GetChildRect(const LayoutContext &lc, const Window &child) const
{
	float scale = lc.GetScale();
	vec2d size = lc.GetPixelSize();

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

	return Window::GetChildRect(lc, child);
}

void Button::Draw(const StateContext &sc, const LayoutContext &lc, const InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	ButtonBase::Draw(sc, lc, ic, dc, texman);

	State state = GetState(lc, ic);
	_background->SetFrame(state);
}


///////////////////////////////////////////////////////////////////////////////
// TextButton

TextButton::TextButton(LayoutManager &manager, TextureManager &texman)
	: ButtonBase(manager)
	, _text(std::make_shared<Text>(manager, texman))
{
	AddFront(_text);
	_text->SetFontColor(c_textColor);
}

vec2d TextButton::GetContentSize(const StateContext &sc, TextureManager &texman) const
{
	return _text->GetContentSize(sc, texman);
}

void TextButton::SetFont(TextureManager &texman, const char *fontName)
{
	_text->SetFont(texman, fontName);
}

void TextButton::SetText(std::shared_ptr<TextSource> text)
{
	_text->SetText(std::move(text));
}

FRECT TextButton::GetChildRect(const LayoutContext &lc, const Window &child) const
{
	if (_text.get() == &child)
	{
		return MakeRectWH(lc.GetPixelSize());
	}

	return ButtonBase::GetChildRect(lc, child);
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

const std::string& CheckBox::GetText() const
{
	return _text;
}

void CheckBox::SetText(TextureManager &texman, const std::string &text)
{
	_text.assign(text);
	OnTextChange(texman);
}

void CheckBox::Draw(const StateContext &sc, const LayoutContext &lc, const InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	ButtonBase::Draw(sc, lc, ic, dc, texman);

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
