#pragma once
#include <math/MyMath.h>

struct IRender;

namespace Plat
{
	struct Clipboard;
	struct Input;
	enum class Key;
	enum class Msg;
	enum class PointerType;

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
		virtual Clipboard& GetClipboard() = 0;
		virtual Input& GetInput() = 0;
		virtual IRender& GetRender() = 0;
		virtual void SetCanNavigateBack(bool canNavigateBack) = 0;
		virtual void SetMouseCursor(MouseCursor mouseCursor) = 0;
		virtual void MakeCurrent() = 0;
		virtual void Present() = 0;
	};

	struct AppWindowInputSink
	{
		virtual bool OnChar(unsigned int codepoint) = 0;
		virtual bool OnKey(Key key, Msg action) = 0;
		virtual bool OnPointer(PointerType pointerType, Msg action, vec2d pxPointerPos, vec2d pxPointerOffset, int buttons, unsigned int pointerID) = 0;
		virtual bool OnSystemNavigationBack() = 0;
		virtual void OnRefresh() = 0;
	};
}
