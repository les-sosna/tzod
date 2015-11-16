#pragma once

namespace UI
{
	struct IInput;
	struct IClipboard;
	class LayoutManager;
}

struct AppWindow
{
	virtual UI::IClipboard& GetClipboard() = 0;
	virtual UI::IInput& GetInput() = 0;
	virtual unsigned int GetPixelWidth() = 0;
	virtual unsigned int GetPixelHeight() = 0;
	virtual void SetInputSink(UI::LayoutManager *inputSink) = 0;
};
