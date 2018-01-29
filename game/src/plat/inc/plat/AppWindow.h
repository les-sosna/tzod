#pragma once
#include <math/MyMath.h>

struct IRender;

namespace UI
{
	struct IInput;
	struct IClipboard;
	enum class Key;
	enum class Msg;
	enum class PointerType;
}

enum class MouseCursor
{
	Arrow,
	IBeam
};

struct AppWindowInputSink;

struct AppWindow
{
	virtual AppWindowInputSink* GetInputSink() const = 0;
	virtual void SetInputSink(AppWindowInputSink *inputSink) = 0;
	virtual int GetDisplayRotation() const = 0;
	virtual vec2d GetPixelSize() const = 0;
	virtual float GetLayoutScale() const = 0;
	virtual UI::IClipboard& GetClipboard() = 0;
	virtual UI::IInput& GetInput() = 0;
	virtual IRender& GetRender() = 0;
	virtual void SetCanNavigateBack(bool canNavigateBack) = 0;
	virtual void SetMouseCursor(MouseCursor mouseCursor) = 0;
	virtual void MakeCurrent() = 0;
	virtual void Present() = 0;
};

struct AppWindowInputSink
{
	virtual bool OnChar(unsigned int codepoint) = 0;
	virtual bool OnKey(UI::Key key, UI::Msg action) = 0;
	virtual bool OnPointer(UI::PointerType pointerType, UI::Msg action, vec2d pxPointerPos, vec2d pxPointerOffset, int buttons, unsigned int pointerID) = 0;
	virtual bool OnSystemNavigationBack() = 0;
	virtual void OnRefresh() = 0;
};
