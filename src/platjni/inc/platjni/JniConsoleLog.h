#pragma once
#include <plat/ConsoleBuffer.h>

class JniConsoleLog final
    : public Plat::IConsoleLog
{
public:
    // Plat::IConsoleLog
    void WriteLine(int severity, std::string_view str) override;
    void Release() override;
};
