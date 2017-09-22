#pragma once
#include <string>

namespace UI
{
	struct IClipboard
	{
		virtual std::string_view GetClipboardText() const = 0;
		virtual void SetClipboardText(std::string text) = 0;
	};
}
