#pragma once

#include <ui/Dialog.h>
#include <memory>
#include <vector>

class ShellConfig;
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

class PropertyList
	: public UI::Dialog
	, private UI::Managerful
{
public:
	PropertyList(UI::LayoutManager &manager, TextureManager &texman, World &world, ShellConfig &conf, UI::ConsoleBuffer &logger, LangCache &lang);
	void ConnectTo(std::shared_ptr<PropertySet> ps, TextureManager &texman);
	void DoExchange(bool applyToObject, TextureManager &texman);

	// UI::Window
	FRECT GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const override;

private:
	std::shared_ptr<UI::Button> _deleteButton;
	std::shared_ptr<UI::ScrollView> _scrollView;
	std::shared_ptr<UI::StackLayout> _psheet; // scrollable content

	std::shared_ptr<PropertySet> _ps;
	std::vector<std::shared_ptr<Window>> _ctrls;
	World &_world;
	ShellConfig &_conf;
	UI::ConsoleBuffer &_logger;

	// UI::KeyboardSink
	bool OnKeyPressed(UI::InputContext &ic, UI::Key key) override;
};

void SaveToConfig(ShellConfig &conf, const PropertySet &ps);
void LoadFromConfig(const ShellConfig &conf, PropertySet &ps);
