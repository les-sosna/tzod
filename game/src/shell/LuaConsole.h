#pragma once
#include <luaetc/LuaDeleter.h>
#include <memory>
#include <string_view>

namespace UI
{
	class ConsoleBuffer;
}

namespace FS
{
	class FileSystem;
}

class ConfVarTable;

class LuaConsole
{
public:
	LuaConsole(UI::ConsoleBuffer &logger, ConfVarTable &configRoot, FS::FileSystem &fs);

	void Exec(std::string_view cmd);
	bool CompleteCommand(std::string_view cmd, int &pos, std::string &result);

private:
	UI::ConsoleBuffer &_logger;
	std::unique_ptr<lua_State, LuaStateDeleter> _L;
};
