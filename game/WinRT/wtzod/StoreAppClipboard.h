#pragma once
#include <ui/Clipboard.h>

class StoreAppClipboard : public UI::IClipboard
{
public:
	// UI::IClipboard
	std::string_view GetClipboardText() const override;
	void SetClipboardText(std::string text) override;
};
