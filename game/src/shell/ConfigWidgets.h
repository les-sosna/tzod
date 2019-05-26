#pragma once
#include <config/ConfigBase.h>
#include <ui/Window.h>

namespace UI
{
	class CheckBox;
	class ContentButton;
	class Edit;
	class Text;
	template<class T> struct LayoutData;
}

class StringSetting : public UI::Window
{
public:
	StringSetting(ConfVarString& stringVar);
	void SetTitle(std::shared_ptr<UI::LayoutData<std::string_view>> title);

	void SetDefaultValue(std::string value) { _defaultValue = std::move(value); }
	std::string_view GetDefaultValue() const { return _defaultValue; }

	// UI::Window
	FRECT GetChildRect(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, const UI::Window& child) const override;
	vec2d GetContentSize(TextureManager& texman, const UI::DataContext& dc, float scale, const UI::LayoutConstraints& layoutConstraints) const override;
	std::shared_ptr<UI::Window> GetFocus() const override;

private:
	ConfVarString& _stringVar;
	std::string _defaultValue;
	std::shared_ptr<UI::Text> _title;
	std::shared_ptr<UI::Edit> _valueEditBox;
};

class BooleanSetting : public UI::Window
{
public:
	BooleanSetting(ConfVarBool& stringVar);
	void SetTitle(std::shared_ptr<UI::LayoutData<std::string_view>> title);

	// UI::Window
	FRECT GetChildRect(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, const UI::Window& child) const override;
	vec2d GetContentSize(TextureManager& texman, const UI::DataContext& dc, float scale, const UI::LayoutConstraints& layoutConstraints) const override;
	std::shared_ptr<UI::Window> GetFocus() const override;

private:
	ConfVarBool& _boolVar;
	std::shared_ptr<UI::CheckBox> _valueCheckBox;
};

class KeyBindSetting final
	: public UI::Window
	, private UI::KeyboardSink
{
public:
	KeyBindSetting(ConfVarString& stringKeyVar);
	void SetTitle(std::shared_ptr<UI::LayoutData<std::string_view>> title);

	// UI::Window
	FRECT GetChildRect(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, const UI::Window& child) const override;
	float GetChildOpacity(const UI::LayoutContext& lc, const UI::InputContext& ic, const UI::Window& child) const override;
	vec2d GetContentSize(TextureManager& texman, const UI::DataContext& dc, float scale, const UI::LayoutConstraints& layoutConstraints) const override;
	std::shared_ptr<UI::Window> GetFocus() const override;
	bool HasKeyboardSink() const override { return true; }
	UI::KeyboardSink* GetKeyboardSink() override { return this; }

private:
	// UI::KeyboardSink
	bool OnKeyPressed(const UI::InputContext& ic, Plat::Key key) override;

	class KeyBindSettingContent : public UI::Window
	{
	public:
		KeyBindSettingContent(const ConfVarString& stringKeyVar);
		void SetTitle(std::shared_ptr<UI::LayoutData<std::string_view>> title);

		// UI::Window
		FRECT GetChildRect(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, const UI::Window& child) const override;
		vec2d GetContentSize(TextureManager& texman, const UI::DataContext& dc, float scale, const UI::LayoutConstraints& layoutConstraints) const override;

	private:
		std::shared_ptr<UI::Text> _title;
		std::shared_ptr<UI::Text> _keyName;
	};

	ConfVarString& _stringKeyVar;
	std::shared_ptr<KeyBindSettingContent> _content;
	std::shared_ptr<UI::ContentButton> _button;
	bool _waitingForKey = false;
};
