#pragma once
#include <ui/Console.h>
#include <string>

class ShellConfig;

class ConfigConsoleHistory final
	: public UI::IConsoleHistory
{
public:
	explicit ConfigConsoleHistory(const ShellConfig &conf);

	// UI::IConsoleHistory
	void Enter(std::string str) override;
	size_t GetItemCount() const override;
	std::string_view GetItem(size_t index) const override;

private:
	const ShellConfig &_conf;
};
