#pragma once
#include <memory>
#include <luaetc/LuaDeleter.h>

class GC_Object;
class World;
namespace Plat
{
	class ConsoleBuffer;
}

class QuickActions
{
public:
	QuickActions(Plat::ConsoleBuffer &logger, World &world);

	void DoAction(GC_Object &object);

private:
	Plat::ConsoleBuffer &_logger;
	std::unique_ptr<lua_State, LuaStateDeleter> _L;
};
