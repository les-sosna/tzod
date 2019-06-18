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

	struct AppWindowCommandClose
	{
		virtual void RequestClose() = 0;
	};

	struct AppWindow
	{
		virtual int GetDisplayRotation() const = 0;
		virtual vec2d GetPixelSize() const = 0;
		virtual float GetLayoutScale() const = 0;
		virtual Clipboard& GetClipboard() = 0;
		virtual Input& GetInput() = 0;
		virtual IRender& GetRender() = 0;
		virtual void SetCanNavigateBack(bool canNavigateBack) = 0;
		virtual void SetMouseCursor(MouseCursor mouseCursor) = 0;
		virtual void Present() = 0;

		virtual AppWindowCommandClose* CmdClose() { return nullptr; }
	};

	struct AppWindowInputSink
	{
		virtual bool OnChar(Plat::AppWindow& appWindow, unsigned int codepoint) = 0;
		virtual bool OnKey(Plat::AppWindow& appWindow, Key key, Msg action) = 0;
		virtual bool OnPointer(Plat::AppWindow& appWindow, PointerType pointerType, Msg action, vec2d pxPointerPos, vec2d pxPointerOffset, int buttons, unsigned int pointerID) = 0;
		virtual bool OnSystemNavigationBack(Plat::AppWindow& appWindow) = 0;
		virtual void OnRefresh(Plat::AppWindow& appWindow) = 0;
	};
}
