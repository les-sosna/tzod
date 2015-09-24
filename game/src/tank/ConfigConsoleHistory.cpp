#include "ConfigConsoleHistory.h"
#include "Config.h"

void ConfigConsoleHistory::Enter(std::string str)
{
	g_conf.con_history.PushBack(ConfVar::typeString)->AsStr()->Set(std::move(str));
	while ((signed)g_conf.con_history.GetSize() > g_conf.con_maxhistory.GetInt())
	{
		g_conf.con_history.PopFront();
	}
}

size_t ConfigConsoleHistory::GetItemCount() const
{
	return g_conf.con_history.GetSize();
}

const std::string& ConfigConsoleHistory::GetItem(size_t index) const
{
	return g_conf.con_history.GetStr(index, "")->Get();
}
