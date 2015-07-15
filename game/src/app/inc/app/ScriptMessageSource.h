#pragma once

struct ScriptMessageSink;

struct ScriptMessageSource
{
    virtual void AddListener(ScriptMessageSink &listener) = 0;
    virtual void RemoveListener(ScriptMessageSink &listener) = 0;
};
