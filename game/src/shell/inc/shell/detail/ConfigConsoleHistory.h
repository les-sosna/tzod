#pragma once
#include <ui/Console.h>
#include <string>

class ConfCache;

class ConfigConsoleHistory : public UI::IConsoleHistory
{
public:
	explicit ConfigConsoleHistory(ConfCache &conf);

	// UI::IConsoleHistory
	virtual void Enter(std::string str);
	virtual size_t GetItemCount() const;
	virtual const std::string& GetItem(size_t index) const;

private:
	ConfCache &_conf;
};
