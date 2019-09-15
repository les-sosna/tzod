#pragma once
#include <config/ConfigBase.h>
#include <ui/Button.h>
#include <ui/Edit.h>
#include <ui/Text.h>
#include <ui/Window.h>

namespace UI
{
	template<class T> struct LayoutData;
}

class StringSetting final
	: public UI::Window
{
	TUPLE_CHILDREN(UI::Text, UI::Edit);
    STATIC_FOCUS(UI::Edit);

public:
	StringSetting(ConfVarString& stringVar);
	void SetTitle(std::shared_ptr<UI::LayoutData<std::string_view>> title);

	void SetDefaultValue(std::string value) { _defaultValue = std::move(value); }
	std::string_view GetDefaultValue() const { return _defaultValue; }

	// UI::Window
	UI::WindowLayout GetChildLayout(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, const UI::Window& child) const override;
	vec2d GetContentSize(TextureManager& texman, const UI::DataContext& dc, float scale, const UI::LayoutConstraints& layoutConstraints) const override;

private:
	ConfVarString& _stringVar;
	std::string _defaultValue;
};

class BooleanSetting : public UI::Window
{
	TUPLE_CHILDREN(UI::CheckBox);
    STATIC_FOCUS(0);

public:
	BooleanSetting(ConfVarBool& stringVar);
	void SetTitle(std::shared_ptr<UI::LayoutData<std::string_view>> title);

	// UI::Window
	UI::WindowLayout GetChildLayout(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, const UI::Window& child) const override;
	vec2d GetContentSize(TextureManager& texman, const UI::DataContext& dc, float scale, const UI::LayoutConstraints& layoutConstraints) const override;

private:
	ConfVarBool& _boolVar;
};

class KeyBindSetting final
	: public UI::Window
	, private UI::KeyboardSink
{
	TUPLE_CHILDREN(UI::ContentButton);
    STATIC_FOCUS(0);

public:
	KeyBindSetting(ConfVarString& stringKeyVar);
	void SetTitle(std::shared_ptr<UI::LayoutData<std::string_view>> title);

	// UI::Window
	UI::WindowLayout GetChildLayout(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, const UI::Window& child) const override;
	vec2d GetContentSize(TextureManager& texman, const UI::DataContext& dc, float scale, const UI::LayoutConstraints& layoutConstraints) const override;
	UI::KeyboardSink* GetKeyboardSink() override { return this; }

private:
	// UI::KeyboardSink
	bool OnKeyPressed(const UI::InputContext& ic, Plat::Key key) override;

	class KeyBindSettingContent : public UI::Window
	{
		//             title     value
		TUPLE_CHILDREN(UI::Text, UI::Text);

	public:
		KeyBindSettingContent(const ConfVarString& stringKeyVar);
		void SetTitle(std::shared_ptr<UI::LayoutData<std::string_view>> title);

		// UI::Window
		UI::WindowLayout GetChildLayout(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, const UI::Window& child) const override;
		vec2d GetContentSize(TextureManager& texman, const UI::DataContext& dc, float scale, const UI::LayoutConstraints& layoutConstraints) const override;
	};

	ConfVarString& _stringKeyVar;
	std::shared_ptr<KeyBindSettingContent> _content;
	bool _waitingForKey = false;
};
