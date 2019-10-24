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
{
	std::get<UI::Text>(_children).SetFont("font_default");

	std::get<UI::Edit>(_children).GetEditable().SetText(std::string(stringVar.Get()));
	std::get<UI::Edit>(_children).GetEditable().SetFont("font_default");
	std::get<UI::Edit>(_children).GetEditable().eventChange = [=]
	{
		auto text = std::get<UI::Edit>(_children).GetEditable().GetText();
		if (text.empty())
			_stringVar.Set(_defaultValue);
		else
			_stringVar.Set(std::get<UI::Edit>(_children).GetEditable().GetText());
	};
}

void StringSetting::SetTitle(std::shared_ptr<UI::LayoutData<std::string_view>> title)
{
	std::get<UI::Text>(_children).SetText(std::move(title));
}

UI::WindowLayout StringSetting::GetChildLayout(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, const UI::Window& child) const
{
	if (&std::get<UI::Text>(_children) == &child)
	{
		return UI::WindowLayout{ MakeRectWH(std::floor(lc.GetPixelSize().x / 2), lc.GetPixelSize().y), 1, true };
	}
	else if (&std::get<UI::Edit>(_children) == &child)
	{
		return UI::WindowLayout{ MakeRectRB(vec2d{ std::floor(lc.GetPixelSize().x / 2), 0 }, lc.GetPixelSize()), 1, true };
	}
	assert(false);
	return {};
}

vec2d StringSetting::GetContentSize(TextureManager& texman, const UI::DataContext& dc, float scale, const UI::LayoutConstraints& layoutConstraints) const
{
	return vec2d{ 0, std::get<UI::Edit>(_children).GetContentSize(texman, dc, scale, layoutConstraints).y };
}

///////////////////////////////////////////////////////////////////////////////

BooleanSetting::BooleanSetting(ConfVarBool& boolVar)
	: _boolVar(boolVar)
{
	std::get<0>(_children).SetFont("font_default");
	std::get<0>(_children).SetBoxPosition(UI::CheckBox::BoxPosition::Right);
	std::get<0>(_children).SetCheck(_boolVar.Get());
	std::get<0>(_children).eventClick = [=]
	{
		_boolVar.Set(std::get<0>(_children).GetCheck());
	};
}

void BooleanSetting::SetTitle(std::shared_ptr<UI::LayoutData<std::string_view>> title)
{
	std::get<0>(_children).SetText(title);
}

UI::WindowLayout BooleanSetting::GetChildLayout(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, const UI::Window& child) const
{
	return UI::WindowLayout{ MakeRectWH(lc.GetPixelSize()), 1, true };
}

vec2d BooleanSetting::GetContentSize(TextureManager& texman, const UI::DataContext& dc, float scale, const UI::LayoutConstraints& layoutConstraints) const
{
	return std::get<0>(_children).GetContentSize(texman, dc, scale, layoutConstraints);
}

///////////////////////////////////////////////////////////////////////////////
//#include <ui/StateContext.h>
#include <ui/DataSource.h>

static const auto c_textColor = std::make_shared<UI::StateBinding<SpriteColor>>(0xffeeeeee, // default
	UI::StateBinding<SpriteColor>::MapType{ { "Disabled", 0xaaaaaaaa }, { "Hover", 0xffffffff }, { "Pushed", 0xffffffff } });

KeyBindSetting::KeyBindSettingContent::KeyBindSettingContent(const ConfVarString& stringKeyVar)
{
	// title
	std::get<0>(_children).SetFont("font_default");
	std::get<0>(_children).SetFontColor(c_textColor);

	// value
	std::get<1>(_children).SetFont("font_default");
	std::get<1>(_children).SetText(ConfBind(stringKeyVar));
	std::get<1>(_children).SetAlign(alignTextRT);
	std::get<1>(_children).SetFontColor(c_textColor);
}

void KeyBindSetting::KeyBindSettingContent::SetTitle(std::shared_ptr<UI::LayoutData<std::string_view>> title)
{
	// title
	std::get<0>(_children).SetText(std::move(title));
}

// UI::Window
UI::WindowLayout KeyBindSetting::KeyBindSettingContent::GetChildLayout(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, const UI::Window& child) const
{
	if (&std::get<0>(_children) == &child)
	{
		return UI::WindowLayout{ MakeRectWH(std::floor(lc.GetPixelSize().x / 2), lc.GetPixelSize().y), 1, true };
	}
	else if (&std::get<1>(_children) == &child)
	{
		return UI::WindowLayout{ MakeRectRB(vec2d{ lc.GetPixelSize().x, 0 }, lc.GetPixelSize()), 1, true };
	}
	assert(false);
	return{};
}

vec2d KeyBindSetting::KeyBindSettingContent::GetContentSize(TextureManager& texman, const UI::DataContext& dc, float scale, const UI::LayoutConstraints& layoutConstraints) const
{
	vec2d pxTitleSize = std::get<0>(_children).GetContentSize(texman, dc, scale, layoutConstraints);
	vec2d pxValueSize = std::get<1>(_children).GetContentSize(texman, dc, scale, layoutConstraints);
	return vec2d{ pxTitleSize.x + pxValueSize.x, std::max(pxTitleSize.y, pxValueSize.y) };
}

KeyBindSetting::KeyBindSetting(ConfVarString& stringKeyVar)
	: _stringKeyVar(stringKeyVar)
	, _content(std::make_shared<KeyBindSettingContent>(stringKeyVar))
{
	std::get<0>(_children).SetContent(_content);
	std::get<0>(_children).eventClick = [=]
	{
		_waitingForKey = true;
	};
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
	return std::get<0>(_children).GetContentSize(texman, dc, scale, layoutConstraints);
}

bool KeyBindSetting::OnKeyPressed(const Plat::Input &input, const UI::InputContext& ic, Plat::Key key)
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
