#include "inc/ui/Button.h"
#include "inc/ui/DataSource.h"
#include "inc/ui/InputContext.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/Rectangle.h"
#include "inc/ui/StateContext.h"
#include "inc/ui/Text.h"
#include "inc/ui/UIInput.h"
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
	StateBinding<unsigned int>::MapType{ { "Disabled", 3 }, { "Hover", 1 }, { "Pushed", 2 } });

Button::Button()
	: _background(std::make_shared<Rectangle>())
	, _text(std::make_shared<Text>())
{
	AddFront(_background);
	AddFront(_text);

	_background->SetFrame(c_backgroundFrame);

	_text->SetAlign(alignTextCC);
	_text->SetFontColor(c_textColor);

	SetBackground("ui/button");
	Resize(96, 24);
}

void Button::SetFont(Texture fontTexture)
{
	_text->SetFont(std::move(fontTexture));
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
		_icon->SetTexture(spriteName);
		_icon->Resize(_icon->GetTextureWidth(texman), _icon->GetTextureHeight(texman));
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

void Button::SetText(std::shared_ptr<LayoutData<std::string_view>> text)
{
	_text->SetText(std::move(text));
}

void Button::SetBackground(Texture background)
{
	_background->SetTexture(std::move(background));
}

const Texture& Button::GetBackground() const
{
	return _background->GetTexture();
}

void Button::AlignToBackground(TextureManager &texman)
{
	Resize(_background->GetTextureWidth(texman), _background->GetTextureHeight(texman));
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

TextButton::TextButton()
	: _text(std::make_shared<Text>())
{
	AddFront(_text);
	_text->SetFontColor(c_textColor);
}

vec2d TextButton::GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const
{
	return _text->GetContentSize(texman, dc, scale);
}

void TextButton::SetFont(Texture fontTexture)
{
	_text->SetFont(std::move(fontTexture));
}

void TextButton::SetText(std::shared_ptr<LayoutData<std::string_view>> text)
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

CheckBox::CheckBox()
	: _text(std::make_shared<Text>())
{
	AddFront(_text);
	_text->SetFontColor(c_textColor);
}

void CheckBox::SetCheck(bool checked)
{
	_isChecked = checked;
}

void CheckBox::OnClick()
{
	SetCheck(!GetCheck());
}

void CheckBox::SetText(std::shared_ptr<LayoutData<std::string_view>> text)
{
	_text->SetText(std::move(text));
}

FRECT CheckBox::GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	if (_text.get() == &child)
	{
		float pxBoxWidth = ToPx(_boxTexture.GetTextureSize(texman), lc).x;
		vec2d pxTextSize = _text->GetContentSize(texman, dc, lc.GetScale());
		return MakeRectWH(vec2d{ pxBoxWidth, std::floor((lc.GetPixelSize().y - pxTextSize.y) / 2) }, pxTextSize);
	}
	return ButtonBase::GetChildRect(texman, lc, dc, child);
}

void CheckBox::Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const
{
	State state = GetState(lc, ic);
	unsigned int frame = _isChecked ? state + 4 : state;

	vec2d pxBoxSize = ToPx(_boxTexture.GetTextureSize(texman), lc);

	auto box = MakeRectWH(vec2d{0, std::floor((lc.GetPixelSize().y - pxBoxSize.y) / 2)}, pxBoxSize);
	rc.DrawSprite(box, _boxTexture.GetTextureId(texman), 0xffffffff, frame);
}

vec2d CheckBox::GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const
{
	vec2d pxTextSize = _text->GetContentSize(texman, dc, scale);
	vec2d pxBoxSize = ToPx(texman.GetFrameSize(_boxTexture.GetTextureId(texman)), scale);
	return vec2d{ pxTextSize.x + pxBoxSize.x, std::max(pxTextSize.y, pxBoxSize.y) };
}
