#pragma once
#include <plat/ConsoleBuffer.h>
#include <fstream>
#include <iostream>

class ConsoleLog final
	: public Plat::IConsoleLog
{
public:
	ConsoleLog(const ConsoleLog&) = delete;
	ConsoleLog& operator= (const ConsoleLog&) = delete;

	explicit ConsoleLog(const char *filename)
		: _file(filename, std::ios::out | std::ios::trunc)
	{
	}

	// IConsoleLog
	void WriteLine(int severity, std::string_view str) override
	{
		_file << str << std::endl;
		std::cout << str << std::endl;
	}
	void Release() override
	{
		delete this;
	}
private:
	std::ofstream _file;
};
