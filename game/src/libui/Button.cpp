#include "inc/ui/Button.h"
#include "inc/ui/DataSource.h"
#include "inc/ui/InputContext.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/Rectangle.h"
#include "inc/ui/StateContext.h"
#include "inc/ui/Text.h"
#include <plat/Input.h>
#include <video/TextureManager.h>
#include <video/RenderContext.h>
#include <algorithm>

using namespace UI;

ButtonBase::State ButtonBase::GetState(const LayoutContext &lc, const InputContext &ic) const
{
	if (!lc.GetEnabledCombined())
		return stateDisabled;

	bool pointerInside = ic.GetPointerType(0) != Plat::PointerType::Unknown && PtInFRect(MakeRectWH(lc.GetPixelSize()), ic.GetPointerPos(0));
	bool pointerPressed = ic.GetInput().GetPointerState(0).pressed;
	if ((pointerInside && pointerPressed && ic.HasCapturedPointers(this)) || ic.GetNavigationSubject(Navigate::Enter).get() == this)
		return statePushed;

	bool focusActive = (ic.GetLastKeyTime() >= ic.GetLastPointerTime()) && ic.GetFocused();
	bool pointerActive = (ic.GetLastPointerTime() > ic.GetLastKeyTime()) && (ic.HasCapturedPointers(this) || (ic.GetHovered() && !pointerPressed));
	if (focusActive || pointerActive)
		return stateHottrack;

	return stateNormal;
}

bool ButtonBase::OnPointerDown(InputContext &ic, LayoutContext &lc, TextureManager &texman, PointerInfo pi, int button)
{
	// touch or primary button only
	return !ic.HasCapturedPointers(this) && (Plat::PointerType::Touch == pi.type || 1 == button);
}

void ButtonBase::OnPointerUp(InputContext &ic, LayoutContext &lc, TextureManager &texman, PointerInfo pi, int button)
{
	if( PtInFRect(MakeRectWH(lc.GetPixelSize()), pi.position) )
	{
		DoClick();
	}
}

void ButtonBase::OnTap(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition)
{
	if( !ic.HasCapturedPointers(this))
	{
		DoClick();
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

void ButtonBase::DoClick()
{
	OnClick();
	if (eventClick)
		eventClick();
}

bool ButtonBase::CanNavigate(Navigate navigate, const LayoutContext &lc, const DataContext &dc) const
{
	return Navigate::Enter == navigate;
}

void ButtonBase::OnNavigate(Navigate navigate, NavigationPhase phase, const LayoutContext &lc, const DataContext &dc)
{
	if (NavigationPhase::Completed == phase && Navigate::Enter == navigate)
	{
		DoClick();
	}
}

///////////////////////////////////////////////////////////////////////////////

static const auto c_textColor = std::make_shared<StateBinding<SpriteColor>>(0xffeeeeee, // default
	StateBinding<SpriteColor>::MapType{ { "Disabled", 0xaaaaaaaa }, { "Hover", 0xffffffff }, { "Pushed", 0xffffffff } });

static const auto c_backgroundFrame = std::make_shared<StateBinding<unsigned int>>(0, // default
	StateBinding<unsigned int>::MapType{ { "Disabled", 3 }, { "Hover", 1 }, { "Pushed", 2 } });

Button::Button()
	: _background(std::make_shared<Rectangle>())
	, _text(std::make_shared<Text>())
{
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
		}
		_icon->SetTexture(spriteName);
		_icon->Resize(_icon->GetTextureWidth(texman), _icon->GetTextureHeight(texman));
	}
	else
	{
		_icon.reset();
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

unsigned int Button::GetChildrenCount() const
{
	return 2 + !!_icon;
}

std::shared_ptr<const Window> Button::GetChild(unsigned int index) const
{
	switch (index)
	{
	default:
		assert(false);
	case 0: return _background;
	case 1: return _text;
	case 2: assert(_icon); return _icon;
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
			pxChildPos.y -= std::floor(_text->GetContentSize(texman, dc, scale, DefaultLayoutConstraints(lc)).y / 2);
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

vec2d TextButton::GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const
{
	return _text->GetContentSize(texman, dc, scale, layoutConstraints);
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

void CheckBox::SetFont(Texture fontTexture)
{
	_text->SetFont(std::move(fontTexture));
}

void CheckBox::SetText(std::shared_ptr<LayoutData<std::string_view>> text)
{
	_text->SetText(std::move(text));
}

FRECT CheckBox::GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	if (_text.get() == &child)
	{
		float pxBoxWidth = _boxPosition == BoxPosition::Left ? ToPx(_boxTexture.GetTextureSize(texman), lc).x : 0;
		vec2d pxTextSize = _text->GetContentSize(texman, dc, lc.GetScale(), DefaultLayoutConstraints(lc));
		return MakeRectWH(vec2d{ pxBoxWidth, std::floor((lc.GetPixelSize().y - pxTextSize.y) / 2) }, pxTextSize);
	}
	return ButtonBase::GetChildRect(texman, lc, dc, child);
}

void CheckBox::Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const
{
	State state = GetState(lc, ic);
	unsigned int frame = _isChecked ? state + 4 : state;

	vec2d pxBoxSize = ToPx(_boxTexture.GetTextureSize(texman), lc);
	float boxLeft = _boxPosition == BoxPosition::Right ? lc.GetPixelSize().x - pxBoxSize.x : 0;
	auto box = MakeRectWH(vec2d{ boxLeft, std::floor((lc.GetPixelSize().y - pxBoxSize.y) / 2)}, pxBoxSize);
	rc.DrawSprite(box, _boxTexture.GetTextureId(texman), 0xffffffff, frame);
}

vec2d CheckBox::GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const
{
	vec2d pxTextSize = _text->GetContentSize(texman, dc, scale, layoutConstraints);
	vec2d pxBoxSize = ToPx(texman.GetFrameSize(_boxTexture.GetTextureId(texman)), scale);
	return vec2d{ pxTextSize.x + pxBoxSize.x, std::max(pxTextSize.y, pxBoxSize.y) };
}
