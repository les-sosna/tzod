#pragma once

#include <ui/Dialog.h>
#include <memory>
#include <vector>

class ConfCache;
class PropertySet;
class TextureManager;
class World;

namespace UI
{
	class ConsoleBuffer;
	class ScrollView;
	class StackLayout;
}

class PropertyList
	: public UI::Dialog
{
public:
	PropertyList(UI::LayoutManager &manager, TextureManager &texman, World &world, ConfCache &_conf, UI::ConsoleBuffer &logger);
	void ConnectTo(std::shared_ptr<PropertySet> ps, TextureManager &texman);
	void DoExchange(bool applyToObject, TextureManager &texman);

	// UI::Window
	FRECT GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::StateContext &sc, const UI::Window &child) const override;

private:
	std::shared_ptr<UI::StackLayout> _psheet;
	std::shared_ptr<UI::ScrollView> _scrollView;

	std::shared_ptr<PropertySet> _ps;
	std::vector<std::shared_ptr<Window>> _ctrls;
	World &_world;
	ConfCache &_conf;
	UI::ConsoleBuffer &_logger;

	// UI::KeyboardSink
	bool OnKeyPressed(UI::InputContext &ic, UI::Key key) override;
};

void SaveToConfig(ConfCache &conf, const PropertySet &ps);
void LoadFromConfig(const ConfCache &conf, PropertySet &ps);
