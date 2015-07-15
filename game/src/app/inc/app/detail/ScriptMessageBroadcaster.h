#pragma once
#include "../ScriptMessageSource.h"
#include <script/ScriptMessageSink.h>
#include <vector>

namespace app_detail
{
    class ScriptMessageBroadcaster
        : public ScriptMessageSink
        , public ScriptMessageSource
    {
    public:
        // ScriptMessageSink
        virtual void ScriptMessage(const char *message) override;

        // ScriptMessageSource
        virtual void AddListener(ScriptMessageSink &listener) override;
        virtual void RemoveListener(ScriptMessageSink &listener) override;

    private:
        std::vector<ScriptMessageSink*> _listeners;
    };
}