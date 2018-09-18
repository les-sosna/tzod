#pragma once
#include <ui/ConsoleBuffer.h>

class JniConsoleLog final
    : public UI::IConsoleLog
{
public:
    // UI::IConsoleLog
    void WriteLine(int severity, std::string_view str) override;
    void Release() override;
};
