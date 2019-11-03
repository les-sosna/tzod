#pragma once
#include <plat/Clipboard.h>

class StoreAppClipboard final
	: public Plat::Clipboard
{
public:
	// Plat::Clipboard
	std::string_view GetClipboardText() const override;
	void SetClipboardText(std::string text) override;
};
