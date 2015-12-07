#pragma once
#include <ui/Clipboard.h>

class StoreAppClipboard : public UI::IClipboard
{
public:
	// UI::IClipboard
	const char* GetClipboardText() const override;
	void SetClipboardText(std::string text) override;
};
