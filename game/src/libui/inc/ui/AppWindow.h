#pragma once

struct IRender;

namespace UI
{
	struct IInput;
	struct IClipboard;
	class LayoutManager;
}

enum class MouseCursor
{
	Arrow,
	IBeam
};

struct AppWindow
{
	virtual UI::IClipboard& GetClipboard() = 0;
	virtual UI::IInput& GetInput() = 0;
	virtual IRender& GetRender() = 0;
	virtual void SetCanNavigateBack(bool canNavigateBack) = 0;
	virtual void SetInputSink(UI::LayoutManager *inputSink) = 0;
	virtual void SetMouseCursor(MouseCursor mouseCursor) = 0;
	virtual void MakeCurrent() = 0;
};
