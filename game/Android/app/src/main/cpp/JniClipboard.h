#pragma once
#include <plat/Clipboard.h>

struct JniClipboard final : public Plat::Clipboard
{
    std::string_view GetClipboardText() const override;
    void SetClipboardText(std::string text) override;
};
