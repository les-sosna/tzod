#pragma once

struct ScriptMessageSink
{
    virtual void ScriptMessage(const char *message) = 0;
};
