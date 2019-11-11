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
	struct AppWindow;
}

namespace UI
{

class DataContext;
class LayoutContext;
class Window;
struct PointerSink;
struct TextSink;

enum class TextOperation
{
	ClipboardCut,
	ClipboardCopy,
	ClipboardPaste,
	CharacterInput
};

class InputContext
{
public:
	void ReadInput(const Plat::Input &input);

	bool ProcessPointer(
		const Plat::Input& input,
		TextureManager &texman,
		std::shared_ptr<Window> wnd,
		const LayoutContext &lc,
		const DataContext &dc,
		vec2d pxPointerPosition,
		vec2d pxPointerOffset,
		Plat::Msg msg,
		int button,
		Plat::PointerType pointerType,
		unsigned int pointerID,
		float time);

	bool ProcessKeys(
		const Plat::Input& input,
		TextureManager &texman,
		std::shared_ptr<Window> wnd,
		const LayoutContext &lc,
		const DataContext &dc,
		Plat::Msg msg,
		Plat::Key key,
		float time);

	bool ProcessText(
		TextureManager &texman,
		std::shared_ptr<Window> wnd,
		Plat::AppWindow &appWindow,
		TextOperation textOperation,
		int codepoint = 0);

	bool ProcessSystemNavigationBack(
		TextureManager &texman,
		std::shared_ptr<Window> wnd,
		const LayoutContext &lc,
		const DataContext &dc);

	TextSink* GetTextSink(TextureManager &texman, std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc);

	Plat::PointerType GetPointerType(unsigned int index) const;
	vec2d GetPointerPos(unsigned int index, const LayoutContext& lc) const;
	std::shared_ptr<Window> GetNavigationSubject(Navigate navigate) const;
	float GetLastKeyTime() const { return _lastKeyTime; }
	float GetLastPointerTime() const { return _lastPointerTime; }

	const std::vector<std::shared_ptr<Window>>* GetCapturePath(unsigned int pointerID) const;
	bool HasCapturedPointers(const Window* wnd) const;

	bool GetMainWindowActive() const { return _isAppActive; }

#ifndef NDEBUG
	const std::unordered_map<unsigned int, vec2d>& GetLastPointerLocation() const { return _lastPointerLocation; }
#endif

private:
	bool ProcessScroll(TextureManager &texman, std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc, vec2d pxPointerPosition, vec2d offset, bool precise);

	Plat::PointerState _pointerState = {};

	float _lastKeyTime = 0;
	float _lastPointerTime = 0;

	struct PointerCapture
	{
		std::vector<std::shared_ptr<Window>> capturePath;
	};

	std::unordered_map<unsigned int, PointerCapture> _pointerCaptures;
	std::unordered_map<Navigate, std::weak_ptr<Window>> _navigationSubjects;

	bool _isAppActive = true;
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
