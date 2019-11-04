#pragma once
#include <gc/Object.h>
#include <ui/Navigation.h>
#include <ui/Window.h>
#include <functional>

class EditorConfig;
class EditorContext;
class EditorWorldView;
class LangCache;
class TextureManager;
class WorldView;

namespace Plat
{
	class ConsoleBuffer;
}

namespace UI
{
	class ListDataSourceDefault;
	class CheckBox;
	class Text;
	class ListBox;
	class Rectangle;
	class StackLayout;
	template<class, class> class ListAdapter;
}

struct EditorCommands
{
	std::function<void()> playMap;
};

class EditorMain
	: public UI::WindowContainer
	, private UI::KeyboardSink
	, private UI::NavigationSink
{
public:
	EditorMain(UI::TimeStepManager &manager,
		TextureManager &texman,
		std::shared_ptr<EditorContext> editorContext,
		WorldView &worldView,
		EditorConfig &conf,
		LangCache &lang,
		EditorCommands commands,
		Plat::ConsoleBuffer &logger);
	virtual ~EditorMain();

	// UI::Window
	UI::WindowLayout GetChildLayout(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const override;
	vec2d GetContentSize(TextureManager& texman, const UI::DataContext& dc, float scale, const UI::LayoutConstraints& layoutConstraints) const override;
	NavigationSink* GetNavigationSink() override { return this; }
	KeyboardSink* GetKeyboardSink() override { return this; }

private:
	void OnSelectType(int selectionIndex);
	void OnChangeUseLayers();
	void ChooseNextType();
	void ChoosePrevType();

	// UI::KeyboardSink
	bool OnKeyPressed(const Plat::Input &input, const UI::InputContext &ic, Plat::Key key) override;

	// UI::NavigationSink
	bool CanNavigate(TextureManager& texman, const UI::InputContext& ic, const UI::LayoutContext& lc, const UI::DataContext& dc, UI::Navigate navigate) const override;
	void OnNavigate(TextureManager& texman, const UI::InputContext& ic, const UI::LayoutContext& lc, const UI::DataContext& dc, UI::Navigate navigate, UI::NavigationPhase phase) override;

	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ListBox> DefaultListBox;
	EditorConfig &_conf;
	LangCache &_lang;
	EditorCommands _commands;
	std::shared_ptr<EditorWorldView> _editorWorldView;
	std::shared_ptr<UI::Text> _layerDisp;
	std::shared_ptr<UI::Window> _helpBox;
	std::shared_ptr<UI::CheckBox> _modeSelect;
	std::shared_ptr<UI::CheckBox> _modeErase;
	std::shared_ptr<DefaultListBox> _typeSelector;
	std::shared_ptr<UI::StackLayout> _toolbar;
	std::vector<ObjectType> _typeSelectorTypes;
};
