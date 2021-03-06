#pragma once
#include <ui/Dialog.h>
#include <memory>
#include <vector>

class EditorConfig;
class LangCache;
class PropertySet;
class TextureManager;
class World;

namespace Plat
{
	class ConsoleBuffer;
}

namespace UI
{
	class Button;
	class ScrollView;
	class StackLayout;
}

class PropertyList : public UI::Dialog
{
public:
	PropertyList(TextureManager &texman, World &world, EditorConfig &conf, Plat::ConsoleBuffer &logger, LangCache &lang);
	void ConnectTo(std::shared_ptr<PropertySet> ps);
	void DoExchange(bool applyToObject);

	// UI::Window
	UI::WindowLayout GetChildLayout(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const override;

private:
	std::shared_ptr<UI::Button> _deleteButton;
	std::shared_ptr<UI::ScrollView> _scrollView;
	std::shared_ptr<UI::StackLayout> _psheet; // scrollable content
	TextureManager &_texman;

	std::shared_ptr<PropertySet> _ps;
	std::vector<std::shared_ptr<Window>> _ctrls;
	World &_world;
	EditorConfig &_conf;
	Plat::ConsoleBuffer &_logger;

	// UI::KeyboardSink
	bool OnKeyPressed(const Plat::Input &input, const UI::InputContext &ic, Plat::Key key) override;
};

void SaveToConfig(EditorConfig &conf, const PropertySet &ps);
void LoadFromConfig(const EditorConfig &conf, PropertySet &ps);
