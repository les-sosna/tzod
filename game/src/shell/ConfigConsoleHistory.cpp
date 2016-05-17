#include "inc/shell/detail/ConfigConsoleHistory.h"
#include "inc/shell/Config.h"

ConfigConsoleHistory::ConfigConsoleHistory(ConfCache &conf)
	: _conf(conf)
{
}

void ConfigConsoleHistory::Enter(std::string str)
{
	_conf.con_history.PushBack(ConfVar::typeString).AsStr().Set(std::move(str));
	while ((signed)_conf.con_history.GetSize() > _conf.con_maxhistory.GetInt())
	{
		_conf.con_history.PopFront();
	}
}

size_t ConfigConsoleHistory::GetItemCount() const
{
	return _conf.con_history.GetSize();
}

const std::string& ConfigConsoleHistory::GetItem(size_t index) const
{
	return _conf.con_history.GetStr(index).Get();
}
