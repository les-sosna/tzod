#include "ConfigWidgets.h"
#include <ui/Button.h>
#include <ui/Edit.h>
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

FRECT StringSetting::GetChildRect(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, const UI::Window& child) const
{
	if (_title.get() == &child)
	{
		return MakeRectWH(std::floor(lc.GetPixelSize().x / 2), lc.GetPixelSize().y);
	}
	else if (_valueEditBox.get() == &child)
	{
		return MakeRectRB(vec2d{ std::floor(lc.GetPixelSize().x / 2), 0 }, lc.GetPixelSize());
	}
	return UI::Window::GetChildRect(texman, lc, dc, child);
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

FRECT BooleanSetting::GetChildRect(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, const UI::Window& child) const
{
	return MakeRectWH(lc.GetPixelSize());
}

vec2d BooleanSetting::GetContentSize(TextureManager& texman, const UI::DataContext& dc, float scale, const UI::LayoutConstraints& layoutConstraints) const
{
	return _valueCheckBox->GetContentSize(texman, dc, scale, layoutConstraints);
}

std::shared_ptr<UI::Window> BooleanSetting::GetFocus() const
{
	return _valueCheckBox;
}
