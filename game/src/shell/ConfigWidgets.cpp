#include "ConfigWidgets.h"
#include "KeyMapper.h"
#include <cbind/ConfigBinding.h>
#include <plat/Keys.h>
#include <ui/Button.h>
#include <ui/Edit.h>
#include <ui/InputContext.h>
#include <ui/Text.h>
#include <ui/EditableText.h>
#include <ui/LayoutContext.h>

StringSetting::StringSetting(ConfVarString& stringVar)
	: _stringVar(stringVar)
	, _title(std::make_shared<UI::Text>())
	, _valueEditBox(std::make_shared<UI::Edit>())
{
	_title->SetFont("font_default");

	_valueEditBox->GetEditable()->SetText(std::string(stringVar.Get()));
	_valueEditBox->GetEditable()->SetFont("font_default");
	_valueEditBox->GetEditable()->eventChange = [=]
	{
		auto text = _valueEditBox->GetEditable()->GetText();
		if (text.empty())
			_stringVar.Set(_defaultValue);
		else
			_stringVar.Set(_valueEditBox->GetEditable()->GetText());
	};
	AddFront(_title);
	AddFront(_valueEditBox);
}

void StringSetting::SetTitle(std::shared_ptr<UI::LayoutData<std::string_view>> title)
{
	_title->SetText(std::move(title));
}

UI::WindowLayout StringSetting::GetChildLayout(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, const UI::Window& child) const
{
	if (_title.get() == &child)
	{
		return UI::WindowLayout{ MakeRectWH(std::floor(lc.GetPixelSize().x / 2), lc.GetPixelSize().y), 1, true };
	}
	else if (_valueEditBox.get() == &child)
	{
		return UI::WindowLayout{ MakeRectRB(vec2d{ std::floor(lc.GetPixelSize().x / 2), 0 }, lc.GetPixelSize()), 1, true };
	}
	return UI::Window::GetChildLayout(texman, lc, dc, child);
}

vec2d StringSetting::GetContentSize(TextureManager& texman, const UI::DataContext& dc, float scale, const UI::LayoutConstraints& layoutConstraints) const
{
	return vec2d{ 0, _valueEditBox->GetContentSize(texman, dc, scale, layoutConstraints).y };
}

std::shared_ptr<UI::Window> StringSetting::GetFocus() const
{
	return _valueEditBox;
}

///////////////////////////////////////////////////////////////////////////////

BooleanSetting::BooleanSetting(ConfVarBool& boolVar)
	: _boolVar(boolVar)
	, _valueCheckBox(std::make_shared<UI::CheckBox>())
{
	_valueCheckBox->SetFont("font_default");
	_valueCheckBox->SetBoxPosition(UI::CheckBox::BoxPosition::Right);
	_valueCheckBox->SetCheck(_boolVar.Get());
	_valueCheckBox->eventClick = [=]
	{
		_boolVar.Set(_valueCheckBox->GetCheck());
	};
	AddFront(_valueCheckBox);
}

void BooleanSetting::SetTitle(std::shared_ptr<UI::LayoutData<std::string_view>> title)
{
	_valueCheckBox->SetText(title);
}

UI::WindowLayout BooleanSetting::GetChildLayout(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, const UI::Window& child) const
{
	return UI::WindowLayout{ MakeRectWH(lc.GetPixelSize()), 1, true };
}

vec2d BooleanSetting::GetContentSize(TextureManager& texman, const UI::DataContext& dc, float scale, const UI::LayoutConstraints& layoutConstraints) const
{
	return _valueCheckBox->GetContentSize(texman, dc, scale, layoutConstraints);
}

std::shared_ptr<UI::Window> BooleanSetting::GetFocus() const
{
	return _valueCheckBox;
}

///////////////////////////////////////////////////////////////////////////////
//#include <ui/StateContext.h>
#include <ui/DataSource.h>

static const auto c_textColor = std::make_shared<UI::StateBinding<SpriteColor>>(0xffeeeeee, // default
	UI::StateBinding<SpriteColor>::MapType{ { "Disabled", 0xaaaaaaaa }, { "Hover", 0xffffffff }, { "Pushed", 0xffffffff } });

KeyBindSetting::KeyBindSettingContent::KeyBindSettingContent(const ConfVarString& stringKeyVar)
	: _title(std::make_shared<UI::Text>())
	, _keyName(std::make_shared<UI::Text>())
{
	_title->SetFont("font_default");
	_title->SetFontColor(c_textColor);
	AddFront(_title);

	_keyName->SetFont("font_default");
	_keyName->SetText(ConfBind(stringKeyVar));
	_keyName->SetAlign(alignTextRT);
	_keyName->SetFontColor(c_textColor);
	AddFront(_keyName);
}

void KeyBindSetting::KeyBindSettingContent::SetTitle(std::shared_ptr<UI::LayoutData<std::string_view>> title)
{
	_title->SetText(std::move(title));
}

// UI::Window
UI::WindowLayout KeyBindSetting::KeyBindSettingContent::GetChildLayout(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, const UI::Window& child) const
{
	if (_title.get() == &child)
	{
		return UI::WindowLayout{ MakeRectWH(std::floor(lc.GetPixelSize().x / 2), lc.GetPixelSize().y), 1, true };
	}
	else if (_keyName.get() == &child)
	{
		return UI::WindowLayout{ MakeRectRB(vec2d{ lc.GetPixelSize().x, 0 }, lc.GetPixelSize()), 1, true };
	}
	return UI::Window::GetChildLayout(texman, lc, dc, child);
}

vec2d KeyBindSetting::KeyBindSettingContent::GetContentSize(TextureManager& texman, const UI::DataContext& dc, float scale, const UI::LayoutConstraints& layoutConstraints) const
{
	vec2d pxTitleSize = _title->GetContentSize(texman, dc, scale, layoutConstraints);
	vec2d pxValueSize = _keyName->GetContentSize(texman, dc, scale, layoutConstraints);
	return vec2d{ pxTitleSize.x + pxValueSize.x, std::max(pxTitleSize.y, pxValueSize.y) };
}

KeyBindSetting::KeyBindSetting(ConfVarString& stringKeyVar)
	: _stringKeyVar(stringKeyVar)
	, _content(std::make_shared<KeyBindSettingContent>(stringKeyVar))
	, _button(std::make_shared<UI::ContentButton>())
{
	_button->SetContent(_content);
	_button->eventClick = [=]
	{
		_waitingForKey = true;
	};
	AddFront(_button);
}

void KeyBindSetting::SetTitle(std::shared_ptr<UI::LayoutData<std::string_view>> title)
{
	_content->SetTitle(std::move(title));
}

UI::WindowLayout KeyBindSetting::GetChildLayout(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, const UI::Window& child) const
{
	return UI::WindowLayout{ MakeRectWH(lc.GetPixelSize()), (lc.GetFocusedCombined() && _waitingForKey) ? 0.5f : 1, true };
}

vec2d KeyBindSetting::GetContentSize(TextureManager& texman, const UI::DataContext& dc, float scale, const UI::LayoutConstraints& layoutConstraints) const
{
	return _button->GetContentSize(texman, dc, scale, layoutConstraints);
}

std::shared_ptr<UI::Window> KeyBindSetting::GetFocus() const
{
	return _button;
}

bool KeyBindSetting::OnKeyPressed(const UI::InputContext& ic, Plat::Key key)
{
	if (_waitingForKey)
	{
		if (Plat::Key::Escape != key)
			_stringKeyVar.Set(GetKeyName(key));
		_waitingForKey = false;
		return true;
	}
	else if (Plat::Key::Delete == key)
	{
		_stringKeyVar.Set("");
	}
	return false;
}
