#pragma once
#include "Navigation.h"
#include "PointerInput.h"
#include <math/MyMath.h>
#include <plat/Input.h>
#include <memory>
#include <stack>
#include <unordered_map>
#include <vector>

class TextureManager;

namespace Plat
{
	struct Clipboard;
}

namespace UI
{

class DataContext;
class LayoutContext;
class Window;
struct PointerSink;
struct TextSink;

class InputContext
{
public:
	InputContext(Plat::Input &input);

	void ReadInput();

	bool ProcessPointer(
		TextureManager &texman,
		std::shared_ptr<Window> wnd,
		const LayoutContext &lc,
		const DataContext &dc,
		vec2d pxPointerPosition,
		vec2d pxPointerOffset,
		Plat::Msg msg,
		int button,
		Plat::PointerType pointerType,
		unsigned int pointerID);
	bool ProcessKeys(TextureManager &texman, std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc, Plat::Msg msg, Plat::Key key, float time);
	bool ProcessSystemNavigationBack(TextureManager &texman, std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc);

	Plat::Input& GetInput() const { return _input; }
	TextSink* GetTextSink(TextureManager &texman, std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc);

	void PushInputTransform(vec2d offset, bool focused, bool hovered);
	void PopInputTransform();

	vec2d GetMousePos() const;
	bool GetFocused() const;
	bool GetHovered() const;
	std::shared_ptr<Window> GetNavigationSubject(Navigate navigate) const;
	float GetLastKeyTime() const { return _lastKeyTime; }

	const std::vector<std::shared_ptr<Window>>* GetCapturePath(unsigned int pointerID) const;
	bool HasCapturedPointers(const Window* wnd) const;

	bool GetMainWindowActive() const { return _isAppActive; }

#ifndef NDEBUG
	const std::unordered_map<unsigned int, vec2d>& GetLastPointerLocation() const { return _lastPointerLocation; }
#endif

private:
	bool ProcessScroll(TextureManager &texman, std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc, vec2d pxPointerPosition, vec2d offset, bool precise);

	Plat::Input &_input;

	Plat::PointerState _pointerState;

	struct InputStackFrame
	{
		vec2d offset;
		bool focused;
		bool hovered;
	};
	std::stack<InputStackFrame> _transformStack;

	float _lastKeyTime = 0;

	struct PointerCapture
	{
		std::vector<std::shared_ptr<Window>> capturePath;
	};

	std::unordered_map<unsigned int, PointerCapture> _pointerCaptures;
	std::unordered_map<Navigate, std::weak_ptr<Window>> _navigationSubjects;

	bool _isAppActive;
#ifndef NDEBUG
	std::unordered_map<unsigned int, vec2d> _lastPointerLocation;
#endif
};

class LayoutContext;

struct AreaSinkSearch
{
	TextureManager &texman;
	const DataContext &dc;
	bool topMostPass;
	std::vector<std::shared_ptr<Window>> outSinkPath;
};

template<class SinkType>
SinkType* FindAreaSink(
	AreaSinkSearch &search,
	std::shared_ptr<Window> wnd,
	const LayoutContext &lc,
	vec2d pxPointerPosition,
	bool insideTopMost);

} // namespace UI
