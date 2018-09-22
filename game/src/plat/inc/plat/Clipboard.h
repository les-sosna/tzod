#pragma once
#include <string>

struct Clipboard
{
	virtual std::string_view GetClipboardText() const = 0;
	virtual void SetClipboardText(std::string text) = 0;
};
