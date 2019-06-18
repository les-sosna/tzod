#include "inc/platjni/JniConsoleLog.h"
#include <android/log.h>

#define  LOG_TAG    "libtzodjni"

void JniConsoleLog::WriteLine(int severity, std::string_view str)
{
    __android_log_print(severity ? ANDROID_LOG_ERROR : ANDROID_LOG_INFO, LOG_TAG,
                        "%.*s\n", static_cast<int>(str.size()), str.data());
}
void JniConsoleLog::Release()
{
    delete this;
}
