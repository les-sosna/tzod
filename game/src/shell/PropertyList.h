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
	class ScrollBarVertical;
}

class PropertyList
	: public UI::Dialog
{
public:
	PropertyList(UI::LayoutManager &manager, TextureManager &texman, float x, float y, float w, float h, World &world, ConfCache &_conf, UI::ConsoleBuffer &logger);
	void ConnectTo(std::shared_ptr<PropertySet> ps, TextureManager &texman);
	void DoExchange(bool applyToObject, TextureManager &texman);

private:
	void OnScroll(float pos);
	void OnSize(float width, float height) override;

	class Container : public UI::Window
	{
	public:
		Container(UI::LayoutManager &manager);
	};

	std::shared_ptr<Container> _psheet;
	std::shared_ptr<UI::ScrollBarVertical> _scrollBar;

	std::shared_ptr<PropertySet>  _ps;
	std::vector<std::shared_ptr<Window>>  _ctrls;
	World &_world;
	ConfCache &_conf;
	UI::ConsoleBuffer &_logger;

	// UI::KeyboardSink
	bool OnKeyPressed(UI::InputContext &ic, UI::Key key) override;

	// UI::PointerSink
	void OnMouseWheel(float x, float y, float z) override;
};

void SaveToConfig(ConfCache &conf, const PropertySet &ps);
void LoadFromConfig(const ConfCache &conf, PropertySet &ps);
