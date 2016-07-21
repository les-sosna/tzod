#pragma once
#include <memory>
#include <luaetc/LuaDeleter.h>

class GC_Object;
class World;
namespace UI
{
	class ConsoleBuffer;
}

class QuickActions
{
public:
	QuickActions(UI::ConsoleBuffer &logger, World &world);

	void DoAction(GC_Object &object);

private:
	UI::ConsoleBuffer &_logger;
	std::unique_ptr<lua_State, LuaStateDeleter> _L;
};
