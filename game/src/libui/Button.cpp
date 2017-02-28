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
#include <video/RenderContext.h>
#include <algorithm>

using namespace UI;

ButtonBase::State ButtonBase::GetState(const LayoutContext &lc, const InputContext &ic) const
{
	if (!lc.GetEnabledCombined())
		return stateDisabled;

	vec2d pointerPosition = ic.GetMousePos();
	bool pointerInside = PtInFRect(MakeRectWH(lc.GetPixelSize()), pointerPosition);
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

static const auto c_textColor = std::make_shared<StateBinding<SpriteColor>>(0xffffffff, // default
	StateBinding<SpriteColor>::MapType{ { "Disabled", 0xbbbbbbbb }, { "Hover", 0xffffffff } });

static const auto c_backgroundFrame = std::make_shared<StateBinding<unsigned int>>(0, // default
	StateBinding<unsigned int>::MapType{ { "Disabled", 3 }, { "Hover", 1 }, {"Pushed", 2} });

Button::Button(TextureManager &texman)
	: _background(std::make_shared<Rectangle>())
	, _text(std::make_shared<Text>(texman))
{
	AddFront(_background);
	AddFront(_text);

	_background->SetFrame(c_backgroundFrame);

	_text->SetAlign(alignTextCC);
	_text->SetFontColor(c_textColor);

	SetFont(texman, "font_small");
	SetBackground(texman, "ui/button", true);
}

void Button::SetFont(TextureManager &texman, const char *fontName)
{
	_text->SetFont(texman, fontName);
}

void Button::SetIcon(TextureManager &texman, const char *spriteName)
{
	if (spriteName)
	{
		if (!_icon)
		{
			_icon = std::make_shared<Rectangle>();
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

void Button::SetText(std::shared_ptr<LayoutData<const std::string&>> text)
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

FRECT Button::GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
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
			pxChildPos.y -= std::floor(_text->GetContentSize(texman, dc, scale).y / 2);
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

	return Window::GetChildRect(texman, lc, dc, child);
}

///////////////////////////////////////////////////////////////////////////////
// TextButton

TextButton::TextButton(TextureManager &texman)
	: _text(std::make_shared<Text>(texman))
{
	AddFront(_text);
	_text->SetFontColor(c_textColor);
}

vec2d TextButton::GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const
{
	return _text->GetContentSize(texman, dc, scale);
}

void TextButton::SetFont(TextureManager &texman, const char *fontName)
{
	_text->SetFont(texman, fontName);
}

void TextButton::SetText(std::shared_ptr<LayoutData<const std::string&>> text)
{
	_text->SetText(std::move(text));
}

FRECT TextButton::GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	if (_text.get() == &child)
	{
		return MakeRectWH(lc.GetPixelSize());
	}

	return ButtonBase::GetChildRect(texman, lc, dc, child);
}

///////////////////////////////////////////////////////////////////////////////

CheckBox::CheckBox(LayoutManager &manager, TextureManager &texman)
  : _fontTexture(texman.FindSprite("font_small"))
  , _boxTexture(texman.FindSprite("ui/checkbox"))
  , _isChecked(false)
{
}

void CheckBox::SetCheck(bool checked)
{
	_isChecked = checked;
}

void CheckBox::OnClick()
{
	SetCheck(!GetCheck());
}

const std::string& CheckBox::GetText() const
{
	return _text;
}

void CheckBox::SetText(const std::string &text)
{
	_text.assign(text);
}

void CheckBox::Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman) const
{
	ButtonBase::Draw(dc, sc, lc, ic, rc, texman);

	State state = GetState(lc, ic);
	size_t frame = _isChecked ? state + 4 : state;

	float bh = texman.GetFrameHeight(_boxTexture, frame);
	float bw = texman.GetFrameWidth(_boxTexture, frame);
	float th = texman.GetFrameHeight(_fontTexture, 0);

	FRECT box = {0, (lc.GetPixelSize().y - bh) / 2, bw, (lc.GetPixelSize().y - bh) / 2 + bh};
	rc.DrawSprite(box, _boxTexture, 0xffffffff, frame);

	// grep 'enum State'
	SpriteColor colors[] =
	{
		SpriteColor(0xffffffff), // Normal
		SpriteColor(0xffffffff), // Hottrack
		SpriteColor(0xffffffff), // Pushed
		SpriteColor(0xffffffff), // Disabled
	};
	rc.DrawBitmapText(vec2d{ bw, (lc.GetPixelSize().y - th) / 2 }, lc.GetScale(), _fontTexture, colors[state], GetText());
}

vec2d CheckBox::GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const
{
	float th = texman.GetFrameHeight(_fontTexture, 0);
	float tw = texman.GetFrameWidth(_fontTexture, 0);
	float bh = texman.GetFrameHeight(_boxTexture, 0);
	float bw = texman.GetFrameWidth(_boxTexture, 0);
	return ToPx(vec2d{ bw + (tw - 1) * (float)GetText().length(), std::max(th + 1, bh) }, scale);
}
