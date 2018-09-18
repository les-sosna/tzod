#pragma once
#include <ui/Clipboard.h>

struct JniClipboard final : public UI::IClipboard
{
    std::string_view GetClipboardText() const override;
    void SetClipboardText(std::string text) override;
};
