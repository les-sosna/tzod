#pragma once

struct IRender;

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
	virtual IRender& GetRender() = 0;
	virtual void SetInputSink(UI::LayoutManager *inputSink) = 0;
	virtual void MakeCurrent() = 0;
};
