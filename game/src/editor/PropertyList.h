#pragma once
#include <ui/Dialog.h>
#include <memory>
#include <vector>

class EditorConfig;
class LangCache;
class PropertySet;
class TextureManager;
class World;

namespace UI
{
	class Button;
	class ConsoleBuffer;
	class ScrollView;
	class StackLayout;
}

class PropertyList : public UI::Dialog
{
public:
	PropertyList(TextureManager &texman, World &world, EditorConfig &conf, UI::ConsoleBuffer &logger, LangCache &lang);
	void ConnectTo(std::shared_ptr<PropertySet> ps);
	void DoExchange(bool applyToObject);

	// UI::Window
	FRECT GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const override;

private:
	std::shared_ptr<UI::Button> _deleteButton;
	std::shared_ptr<UI::ScrollView> _scrollView;
	std::shared_ptr<UI::StackLayout> _psheet; // scrollable content
	TextureManager &_texman;

	std::shared_ptr<PropertySet> _ps;
	std::vector<std::shared_ptr<Window>> _ctrls;
	World &_world;
	EditorConfig &_conf;
	UI::ConsoleBuffer &_logger;

	// UI::KeyboardSink
	bool OnKeyPressed(UI::InputContext &ic, UI::Key key) override;
};

void SaveToConfig(EditorConfig &conf, const PropertySet &ps);
void LoadFromConfig(const EditorConfig &conf, PropertySet &ps);
