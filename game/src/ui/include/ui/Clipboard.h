#pragma once
#include <string>

namespace UI
{
	struct IClipboard
	{
		virtual const char* GetClipboardText() const = 0;
		virtual void SetClipboardText(std::string text) = 0;
	};
}
