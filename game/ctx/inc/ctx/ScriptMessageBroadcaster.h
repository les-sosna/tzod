#pragma once
#include "ScriptMessageSource.h"
#include <script/ScriptMessageSink.h>
#include <vector>

namespace app_detail
{
    class ScriptMessageBroadcaster final
        : public ScriptMessageSink
        , public ScriptMessageSource
    {
    public:
        // ScriptMessageSink
        void ScriptMessage(const char *message) override;

        // ScriptMessageSource
        void AddListener(ScriptMessageSink &listener) override;
        void RemoveListener(ScriptMessageSink &listener) override;

    private:
        std::vector<ScriptMessageSink*> _listeners;
    };
}
