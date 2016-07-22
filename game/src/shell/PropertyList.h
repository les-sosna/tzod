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
}

class PropertyList
	: public UI::Dialog
{
public:
	PropertyList(UI::LayoutManager &manager, TextureManager &texman, float w, float h, World &world, ConfCache &_conf, UI::ConsoleBuffer &logger);
	void ConnectTo(std::shared_ptr<PropertySet> ps, TextureManager &texman);
	void DoExchange(bool applyToObject, TextureManager &texman);

private:
	void OnSize(float width, float height) override;

	std::shared_ptr<UI::Window> _psheet;
	std::shared_ptr<UI::ScrollView> _scrollView;

	std::shared_ptr<PropertySet>  _ps;
	std::vector<std::shared_ptr<Window>>  _ctrls;
	World &_world;
	ConfCache &_conf;
	UI::ConsoleBuffer &_logger;

	// UI::KeyboardSink
	bool OnKeyPressed(UI::InputContext &ic, UI::Key key) override;
};

void SaveToConfig(ConfCache &conf, const PropertySet &ps);
void LoadFromConfig(const ConfCache &conf, PropertySet &ps);
