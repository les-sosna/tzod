#include "inc/ui/DataContext.h"
#include "inc/ui/InputContext.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/Navigation.h"
#include "inc/ui/Window.h"
#include "inc/ui/WindowIterator.h"
#include <plat/AppWindow.h>
#include <plat/Clipboard.h>
#include <plat/Input.h>
#include <plat/Keys.h>

using namespace UI;

struct TraverseFocusPathSettings
{
	TextureManager &texman;
	const InputContext &ic;
	std::function<bool(std::shared_ptr<Window>, const LayoutContext &lc, const DataContext &dc)> visitor;
};

static bool TraverseFocusPath(std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc, const TraverseFocusPathSettings &settings)
{
	if (lc.GetEnabledCombined() && wnd->GetVisible())
	{
		if (auto focusedChild = wnd->GetFocus())
		{
			auto childLayout = wnd->GetChildLayout(settings.texman, lc, dc, *focusedChild);
			LayoutContext childLC(settings.ic, *wnd, lc, *focusedChild, childLayout);

			if (TraverseFocusPath(std::move(focusedChild), childLC, dc, settings))
			{
				return true;
			}
		}

		return settings.visitor(std::move(wnd), lc, dc);
	}

	return false;
}

InputContext::InputContext(Plat::Input &input)
	: _input(input)
	, _isAppActive(true)
#ifndef NDEBUG
	, _lastPointerLocation()
#endif
{
}

void InputContext::ReadInput()
{
	_pointerState = _input.GetPointerState(0);
}

TextSink* InputContext::GetTextSink(TextureManager &texman, std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc)
{
	TextSink *result = nullptr;
	TraverseFocusPath(wnd, lc, dc,
		TraverseFocusPathSettings{
			texman,
			*this,
			[&result](std::shared_ptr<Window> wnd, auto lc, auto dc) // visitor
			{
				result = wnd->GetTextSink();
				return !!result;
			}
		});
	return result;
}

Plat::PointerType InputContext::GetPointerType(unsigned int index) const
{
	if (index == 0)
	{
		return _pointerState.type;
	}
	else
	{
		return Plat::PointerType::Unknown;
	}
}

vec2d InputContext::GetPointerPos(unsigned int index, const LayoutContext& lc) const
{
	assert(GetPointerType(index) != Plat::PointerType::Unknown);
	return _pointerState.position - lc.GetPixelOffsetCombined();
}

std::shared_ptr<Window> InputContext::GetNavigationSubject(Navigate navigate) const
{
	auto found = _navigationSubjects.find(navigate);
	return _navigationSubjects.end() == found ? nullptr : found->second.lock();
}

const std::vector<std::shared_ptr<Window>>* InputContext::GetCapturePath(unsigned int pointerID) const
{
	auto it = _pointerCaptures.find(pointerID);
	return _pointerCaptures.end() != it ? &it->second.capturePath : nullptr;
}

bool InputContext::HasCapturedPointers(const Window *wnd) const
{
	for (auto &capture : _pointerCaptures)
	{
		if (capture.second.capturePath.front().get() == wnd)
		{
			return true;
		}
	}
	return false;
}

namespace
{
	template <class T>
	struct SinkGetter {};

	template <>
	struct SinkGetter<PointerSink>
	{
		static PointerSink* GetSink(Window &wnd)
		{
			return wnd.GetPointerSink();
		}
	};

	template <>
	struct SinkGetter<ScrollSink>
	{
		static ScrollSink* GetSink(Window &wnd)
		{
			return wnd.GetScrollSink();
		}
	};
}

template<class SinkType>
SinkType* UI::FindAreaSink(
	AreaSinkSearch &search,
	std::shared_ptr<Window> wnd,
	const LayoutContext &lc,
	vec2d pxPointerPosition,
	bool insideTopMost)
{
	SinkType *sink = nullptr;

	bool pointerInside = PtInFRect(MakeRectWH(lc.GetPixelSize()), pxPointerPosition);

	if (pointerInside || !wnd->GetClipChildren())
	{
		for (auto child : reverse(*wnd))
		{
			// early skip topmost subtree
			bool childInsideTopMost = insideTopMost || child->GetTopMost();
			if (!childInsideTopMost || search.topMostPass)
			{
				auto childLayout = wnd->GetChildLayout(search.texman, lc, search.dc, *child);
				if (childLayout.enabled && child->GetVisible())
				{
					LayoutContext childLC(search.ic, *wnd, lc, *child, childLayout);
					sink = FindAreaSink<SinkType>(search, child, childLC, pxPointerPosition - Offset(childLayout.rect), childInsideTopMost);
					if (sink)
					{
						break;
					}
				}
			}
		}
	}

	if (!sink && insideTopMost == search.topMostPass && pointerInside)
		sink = SinkGetter<SinkType>::GetSink(*wnd);

	if (sink)
		search.outSinkPath.push_back(wnd);

	return sink;
}

static void PropagateFocus(const std::vector<std::shared_ptr<Window>> &path)
{
	for (size_t i = path.size() - 1; i > 0; i--)
		path[i]->SetFocus(path[i - 1]);
}

static LayoutContext RestoreLayoutContext(TextureManager& texman, const InputContext& ic, LayoutContext lc, const DataContext &dc, const std::vector<std::shared_ptr<Window>> &path)
{
	for (size_t i = path.size() - 1; i > 0; i--)
	{
		auto &child = path[i - 1];
		auto &parent = path[i];
		auto childLayout = parent->GetChildLayout(texman, lc, dc, *child);
		lc = LayoutContext(ic, *parent, lc, *child, childLayout);
	}
	return lc;
}

bool InputContext::ProcessPointer(
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
	float time)
{
#ifndef NDEBUG
	_lastPointerLocation[pointerID] = pxPointerPosition;
#endif

	_lastPointerTime = time;

	if (Plat::Msg::Scroll == msg || Plat::Msg::ScrollPrecise == msg)
	{
		return ProcessScroll(texman, wnd, lc, dc, pxPointerPosition,
			pxPointerOffset / lc.GetScaleCombined(), Plat::Msg::ScrollPrecise == msg);
	}

	PointerSink *pointerSink = nullptr;
	std::vector<std::shared_ptr<Window>> sinkPath;

	const auto capturedIt = _pointerCaptures.find(pointerID);
	bool isPointerCaptured = _pointerCaptures.end() != capturedIt;
	if (isPointerCaptured)
	{
		pointerSink = capturedIt->second.capturePath.front()->GetPointerSink();
		assert(pointerSink);
		sinkPath = capturedIt->second.capturePath;
	}
	else
	{
		for (bool topMostPass : {true, false})
		{
			AreaSinkSearch search{ texman, *this, dc, topMostPass };
			pointerSink = FindAreaSink<PointerSink>(search, wnd, lc, pxPointerPosition, wnd->GetTopMost());
			if (pointerSink)
			{
				sinkPath = std::move(search.outSinkPath);
				break;
			}
		}
	}

	if (pointerSink)
	{
		auto &target = sinkPath.front();
		auto targetLC = RestoreLayoutContext(texman, *this, lc, dc, sinkPath);

		if ((Plat::Msg::PointerDown == msg || Plat::Msg::TAP == msg) && NeedsFocus(texman, targetLC, dc, target.get()))
		{
			PropagateFocus(sinkPath);
			targetLC = RestoreLayoutContext(texman, *this, lc, dc, sinkPath); // layout may have changed due to focus change
		}

		PointerInfo pi;
		pi.position = pxPointerPosition - targetLC.GetPixelOffsetCombined();
		pi.type = pointerType;
		pi.id = pointerID;

		switch (msg)
		{
		case Plat::Msg::PointerDown:
			if (pointerSink->OnPointerDown(*this, targetLC, texman, pi, button))
			{
				_pointerCaptures[pointerID].capturePath = std::move(sinkPath);
			}
			break;
		case Plat::Msg::PointerUp:
		case Plat::Msg::PointerCancel:
			if (isPointerCaptured)
			{
				// hold strong ref to sink target and let everyhing else go away
				auto target = std::move(sinkPath.front());
				sinkPath.clear();
				_pointerCaptures.erase(pointerID);
				pointerSink->OnPointerUp(*this, targetLC, texman, pi, button);
			}
			break;
		case Plat::Msg::PointerMove:
			pointerSink->OnPointerMove(*this, targetLC, texman, pi, isPointerCaptured);
			break;
		case Plat::Msg::TAP:
			pointerSink->OnTap(*this, targetLC, texman, pi.position);
			break;
		default:
			assert(false);
		}
	}

	return !!pointerSink;
}

bool InputContext::ProcessScroll(TextureManager &texman, std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc, vec2d pxPointerPosition, vec2d scrollOffset, bool precise)
{
	ScrollSink *scrollSink = nullptr;
	std::vector<std::shared_ptr<Window>> scrollSinkPath;

	// look for topmost windows first
	for (bool topMostPass : {true, false})
	{
		AreaSinkSearch search{ texman, *this, dc, topMostPass };
		scrollSink = FindAreaSink<ScrollSink>(search, wnd, lc, pxPointerPosition, wnd->GetTopMost());
		if (scrollSink)
		{
			scrollSinkPath = std::move(search.outSinkPath);
			break;
		}
	}

	if (scrollSink)
	{
		auto childLC = RestoreLayoutContext(texman, *this, lc, dc, scrollSinkPath);
		pxPointerPosition -= childLC.GetPixelOffsetCombined();
		scrollSink->OnScroll(texman, *this, childLC, dc, scrollOffset, precise);
		return true;
	}

	return false;
}

static std::shared_ptr<Window> NavigateMostDescendantFocus(TextureManager &texman, const InputContext& ic, std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc, Navigate navigate, NavigationPhase phase)
{
	if (wnd->GetVisible())
	{
		std::shared_ptr<Window> handledBy;
		if (auto focusedChild = wnd->GetFocus())
		{
			auto childLayout = wnd->GetChildLayout(texman, lc, dc, *focusedChild);
			if (childLayout.enabled)
			{
				LayoutContext childLC(ic, *wnd, lc, *focusedChild, childLayout);
				handledBy = NavigateMostDescendantFocus(texman, ic, std::move(focusedChild), childLC, dc, navigate, phase);
			}
		}

		if (handledBy)
		{
			return handledBy;
		}
		else if (auto navigationSink = wnd->GetNavigationSink(); navigationSink && navigationSink->CanNavigate(texman, ic, lc, dc, navigate))
		{
			navigationSink->OnNavigate(texman, ic, lc, dc, navigate, phase);
			return wnd;
		}
	}
	return nullptr;
}

static FRECT EnsureVisibleMostDescendantFocus(TextureManager &texman, const InputContext& ic, std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc)
{
	if (auto focusedChild = wnd->GetFocus())
	{
		auto childLayout = wnd->GetChildLayout(texman, lc, dc, *focusedChild);
		LayoutContext childLC(ic, *wnd, lc, *focusedChild, childLayout);

		FRECT pxFocusRect = EnsureVisibleMostDescendantFocus(texman, ic, focusedChild, childLC, dc);

		if (auto scrollSink = wnd->GetScrollSink())
		{
			scrollSink->EnsureVisible(texman, lc, dc, pxFocusRect);

			// Get updated layout as it may have changed due to scroll offset change
			childLayout = wnd->GetChildLayout(texman, lc, dc, *focusedChild);
		}

		return RectOffset(pxFocusRect, Offset(childLayout.rect));
	}
	else
	{
		return MakeRectWH(lc.GetPixelSize());
	}
}

static Navigate GetNavigateAction(Plat::Key key, bool alt, bool shift)
{
	switch (key)
	{
	case Plat::Key::Enter:
	case Plat::Key::NumEnter:
	case Plat::Key::Space:
	case Plat::Key::GamepadA:
		return Navigate::Enter;

	case Plat::Key::Backspace:
	case Plat::Key::Escape:
	case Plat::Key::GamepadB:
		return Navigate::Back;

	case Plat::Key::Tab:
		return shift ? Navigate::Prev : Navigate::Next;

	case Plat::Key::Up:
	case Plat::Key::GamepadLeftThumbstickUp:
	case Plat::Key::GamepadDPadUp:
		return Navigate::Up;

	case Plat::Key::Down:
	case Plat::Key::GamepadLeftThumbstickDown:
	case Plat::Key::GamepadDPadDown:
		return Navigate::Down;

	case Plat::Key::Left:
		if (alt)
		{
			return Navigate::Back;
		}
		// fallthrough
	case Plat::Key::GamepadLeftThumbstickLeft:
	case Plat::Key::GamepadDPadLeft:
		return Navigate::Left;

	case Plat::Key::Right:
	case Plat::Key::GamepadLeftThumbstickRight:
	case Plat::Key::GamepadDPadRight:
		return Navigate::Right;

	case Plat::Key::Home:
		return Navigate::Begin;

	case Plat::Key::End:
		return Navigate::End;

	case Plat::Key::GamepadMenu:
		return Navigate::Menu;

	default:
		return Navigate::None;
	}
}

static bool AllowNonActiveNavigation(Navigate navigate)
{
	return navigate == Navigate::Back ||
		navigate == Navigate::Menu;
}

bool InputContext::ProcessKeys(TextureManager &texman, std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc, Plat::Msg msg, Plat::Key key, float time)
{
	bool currentlyActiveInputMethod = _lastKeyTime >= _lastPointerTime;

	if (key != Plat::Key::LeftShift && key != Plat::Key::RightShift && key != Plat::Key::LeftCtrl && key != Plat::Key::RightCtrl)
	{
		_lastKeyTime = time;
	}

	bool handled = false;

	// raw key events
	switch (msg)
	{
	case Plat::Msg::KeyReleased:
		handled = TraverseFocusPath(wnd, lc, dc, TraverseFocusPathSettings {
			texman,
			*this,
			[key, this](std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc) // visitor
			{
				if (KeyboardSink *keyboardSink = wnd->GetKeyboardSink())
				{
					keyboardSink->OnKeyReleased(*this, key);
				}
				return false;
			}
		});
		break;
	case Plat::Msg::KeyPressed:
		handled = TraverseFocusPath(wnd, lc, dc, TraverseFocusPathSettings {
			texman,
			*this,
			[key, this](std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc) // visitor
			{
				KeyboardSink *keyboardSink = wnd->GetKeyboardSink();
				return keyboardSink ? keyboardSink->OnKeyPressed(*this, key) : false;
			}
		});
		break;
	default:
		assert(false);
	}

	// navigation
	if (!handled)
	{
		bool alt = GetInput().IsKeyPressed(Plat::Key::LeftAlt) || GetInput().IsKeyPressed(Plat::Key::RightAlt);
		bool shift = GetInput().IsKeyPressed(Plat::Key::LeftShift) || GetInput().IsKeyPressed(Plat::Key::RightShift);
		Navigate navigate = GetNavigateAction(key, alt, shift);
		if (Plat::Msg::KeyReleased == msg)
		{
			if (auto wnd = _navigationSubjects[navigate].lock())
			{
				// FIXME: reconstruct the actual LC
				_navigationSubjects[navigate].reset();
				auto navigationSink = wnd->GetNavigationSink();
				assert(navigationSink);
				if (navigationSink->CanNavigate(texman, *this, lc, dc, navigate))
					navigationSink->OnNavigate(texman, *this, lc, dc, navigate, NavigationPhase::Completed);
				handled = true;
			}
		}
		else if (currentlyActiveInputMethod || AllowNonActiveNavigation(navigate))
		{
			auto handledBy = NavigateMostDescendantFocus(texman, *this, wnd, lc, dc, navigate, NavigationPhase::Started);
			handled = !!handledBy;
			_navigationSubjects[navigate] = std::move(handledBy);

			if (handled)
			{
				EnsureVisibleMostDescendantFocus(texman, *this, wnd, lc, dc);
			}
		}
	}

	return handled;
}

bool InputContext::ProcessText(
	TextureManager &texman,
	std::shared_ptr<Window> wnd,
	Plat::AppWindow &appWindow,
	TextOperation textOperation,
	int codepoint)
{
	DataContext dataContext;
	LayoutContext layoutContext(1.f, appWindow.GetLayoutScale(), vec2d{}, appWindow.GetPixelSize(), true /* enabled */, true /* focused */);

	bool modified = false;
	bool handled = TraverseFocusPath(wnd, layoutContext, dataContext,
		TraverseFocusPathSettings{
			texman,
			*this,
			[&appWindow, textOperation, codepoint, &modified](std::shared_ptr<Window> focusWnd, const LayoutContext &focusLC, const DataContext &focusDC) // visitor
			{
				if (focusWnd->HasTextSink())
				{
					TextSink *textSink = focusWnd->GetTextSink();
					switch (textOperation)
					{
					case TextOperation::ClipboardCut:
						if (auto text = textSink->OnCut(); !text.empty())
						{
							appWindow.GetClipboard().SetClipboardText(std::move(text));
							modified = true;
						}
						break;

					case TextOperation::ClipboardCopy:
						if (auto text = textSink->OnCopy(); !text.empty())
						{
							appWindow.GetClipboard().SetClipboardText(std::string(text));
						}
						break;

					case TextOperation::ClipboardPaste:
						if (auto text = appWindow.GetClipboard().GetClipboardText(); !text.empty())
						{
							textSink->OnPaste(text);
							modified = true;
						}
						break;

					case TextOperation::CharacterInput:
						modified = textSink->OnChar(codepoint);
						return modified;
					}

					return true;
				}
				return false;
			}
		});

	if (modified)
	{
		EnsureVisibleMostDescendantFocus(texman, *this, wnd, layoutContext, dataContext);
	}

	return handled;
}

bool InputContext::ProcessSystemNavigationBack(TextureManager &texman, std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc)
{
	auto startedHandledBy = NavigateMostDescendantFocus(texman, *this, wnd, lc, dc, Navigate::Back, NavigationPhase::Started);
	if (startedHandledBy)
	{
		// FIXME: reconstruct the actual LC
		startedHandledBy->GetNavigationSink()->OnNavigate(texman, *this, lc, dc, Navigate::Back, NavigationPhase::Completed);
	}
	return !!startedHandledBy;
}
