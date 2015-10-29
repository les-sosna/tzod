#pragma once

#include <ui/Dialog.h>
#include <memory>
#include <vector>

class ConfCache;
class PropertySet;
class World;

namespace UI
{
	class ConsoleBuffer;
	class ScrollBarVertical;
}

class PropertyList : public UI::Dialog
{
public:
	PropertyList(Window *parent, float x, float y, float w, float h, World &world, ConfCache &_conf, UI::ConsoleBuffer &logger);
	void ConnectTo(std::shared_ptr<PropertySet> ps);
	void DoExchange(bool applyToObject);

private:
	void OnScroll(float pos);
	void OnSize(float width, float height);
	bool OnKeyPressed(UI::Key key);
	bool OnMouseWheel(float x, float y, float z);

	class Container : public UI::Window
	{
//		bool OnKeyPressed(Key key); // need to pass messages through
	public:
		Container(UI::Window *parent);
	};

	Container *_psheet;
	UI::ScrollBarVertical *_scrollBar;

	std::shared_ptr<PropertySet>  _ps;
	std::vector<Window*>  _ctrls;
	World &_world;
	ConfCache &_conf;
	UI::ConsoleBuffer &_logger;
};

void SaveToConfig(ConfCache &conf, const PropertySet &ps);
void LoadFromConfig(const ConfCache &conf, PropertySet &ps);

