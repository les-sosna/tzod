#include "inc/ctx/ScriptMessageBroadcaster.h"
#include <algorithm>
#include <cassert>

using namespace app_detail;

void ScriptMessageBroadcaster::AddListener(ScriptMessageSink &listener)
{
    assert(std::find(_listeners.begin(), _listeners.end(), &listener) == _listeners.end());
    _listeners.push_back(&listener);
}

void ScriptMessageBroadcaster::RemoveListener(ScriptMessageSink &listener)
{
    _listeners.erase(std::find(_listeners.begin(), _listeners.end(), &listener));
}

void ScriptMessageBroadcaster::ScriptMessage(const char *message)
{
    for (auto sink: _listeners)
    {
        sink->ScriptMessage(message);
    }
}
