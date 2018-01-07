#include "inc/ui/DataContext.h"
#include "inc/ui/InputContext.h"
#include "inc/ui/Keys.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/Navigation.h"
#include "inc/ui/UIInput.h"
#include "inc/ui/Window.h"
#include "inc/ui/WindowIterator.h"

using namespace UI;

InputContext::InputContext(IInput &input, IClipboard &clipboard)
	: _input(input)
	, _clipboard(clipboard)
	, _isAppActive(true)
#ifndef NDEBUG
	, _lastPointerLocation()
#endif
{
	_transformStack.emplace(InputStackFrame{vec2d{}, true, true});
}

void InputContext::ReadInput()
{
	_mousePos = _input.GetMousePos();
}

void InputContext::PushInputTransform(vec2d offset, bool focused, bool hovered)
{
	assert(!_transformStack.empty());
	_transformStack.push(InputStackFrame{
		_transformStack.top().offset + offset,
		_transformStack.top().focused && focused,
		_transformStack.top().hovered && hovered
	});
}

void InputContext::PopInputTransform()
{
	assert(_transformStack.size() > 1);
	_transformStack.pop();
}

vec2d InputContext::GetMousePos() const
{
	return _mousePos - _transformStack.top().offset;
}

bool InputContext::GetFocused() const
{
	return _transformStack.top().focused;
}

bool InputContext::GetHovered() const
{
	return _transformStack.top().hovered;
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
		for (auto &child : reverse(*wnd))
		{
			if (child->GetEnabled(search.dc) && child->GetVisible())
			{
				// early skip topmost subtree
				bool childInsideTopMost = insideTopMost || child->GetTopMost();
				if (!childInsideTopMost || search.topMostPass)
				{
					auto childRect = wnd->GetChildRect(search.texman, lc, search.dc, *child);
					LayoutContext childLC(*wnd, lc, *child, Size(childRect), search.dc);
					sink = FindAreaSink<SinkType>(search, child, childLC, pxPointerPosition - Offset(childRect), childInsideTopMost);

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

static std::pair<vec2d, LayoutContext> RestoreOffsetAndLayoutContext(LayoutContext lc, const DataContext &dc, TextureManager &texman, const std::vector<std::shared_ptr<Window>> &path)
{
	vec2d offset{};
	for (size_t i = path.size() - 1; i > 0; i--)
	{
		auto &child = path[i - 1];
		auto &parent = path[i];

		auto childRect = parent->GetChildRect(texman, lc, dc, *child);
		offset += Offset(childRect);
		lc = LayoutContext(*parent, lc, *child, Size(childRect), dc);
	}
	return{ offset, lc };
}

bool InputContext::ProcessPointer(
	TextureManager &texman,
	std::shared_ptr<Window> wnd,
	const LayoutContext &lc,
	const DataContext &dc,
	vec2d pxPointerPosition,
	vec2d pxPointerOffset,
	Msg msg,
	int button,
	PointerType pointerType,
	unsigned int pointerID)
{
#ifndef NDEBUG
	_lastPointerLocation[pointerID] = pxPointerPosition;
#endif

	if (Msg::Scroll == msg)
	{
		return ProcessScroll(texman, wnd, lc, dc, pxPointerPosition, pxPointerOffset);
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
			AreaSinkSearch search{ texman, dc, topMostPass };
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

		if ((Msg::PointerDown == msg || Msg::TAP == msg) && NeedsFocus(target.get(), dc))
			PropagateFocus(sinkPath);

		auto childOffsetAndLC = RestoreOffsetAndLayoutContext(lc, dc, texman, sinkPath);

		PointerInfo pi;
		pi.position = pxPointerPosition - childOffsetAndLC.first;
		pi.type = pointerType;
		pi.id = pointerID;

		switch (msg)
		{
		case Msg::PointerDown:
			if (pointerSink->OnPointerDown(*this, childOffsetAndLC.second, texman, pi, button))
			{
				_pointerCaptures[pointerID].capturePath = std::move(sinkPath);
			}
			break;
		case Msg::PointerUp:
		case Msg::PointerCancel:
			if (isPointerCaptured)
			{
				// hold strong ref to sink target and let everyhing else go away
				auto target = std::move(sinkPath.front());
				sinkPath.clear();
				_pointerCaptures.erase(pointerID);
				pointerSink->OnPointerUp(*this, childOffsetAndLC.second, texman, pi, button);
			}
			break;
		case Msg::PointerMove:
			pointerSink->OnPointerMove(*this, childOffsetAndLC.second, texman, pi, isPointerCaptured);
			break;
		case Msg::TAP:
			pointerSink->OnTap(*this, childOffsetAndLC.second, texman, pi.position);
			break;
		default:
			assert(false);
		}
	}

	return !!pointerSink;
}

bool InputContext::ProcessScroll(TextureManager &texman, std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc, vec2d pxPointerPosition, vec2d scrollOffset)
{
	ScrollSink *scrollSink = nullptr;
	std::vector<std::shared_ptr<Window>> sinkPath;

	// look for topmost windows first
	for (bool topMostPass : {true, false})
	{
		AreaSinkSearch search{ texman, dc, topMostPass };
		scrollSink = FindAreaSink<ScrollSink>(search, wnd, lc, pxPointerPosition, wnd->GetTopMost());
		if (scrollSink)
		{
			sinkPath = std::move(search.outSinkPath);
			break;
		}
	}

	if (scrollSink)
	{
		auto childOffsetAndLC = RestoreOffsetAndLayoutContext(lc, dc, texman, sinkPath);
		pxPointerPosition -= childOffsetAndLC.first;
		scrollSink->OnScroll(texman, *this, childOffsetAndLC.second, dc, scrollOffset);
		return true;
	}

	return false;
}


struct TraverseFocusPathSettings
{
	TextureManager &texman;
	InputContext &ic;
	std::function<bool(std::shared_ptr<Window>)> visitor;
};

static bool TraverseFocusPath(std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc, const TraverseFocusPathSettings &settings)
{
	if (lc.GetEnabledCombined() && wnd->GetVisible())
	{
		if (auto focusedChild = wnd->GetFocus())
		{
			auto childRect = wnd->GetChildRect(settings.texman, lc, dc, *focusedChild);
			LayoutContext childLC(*wnd, lc, *focusedChild, Size(childRect), dc);

			if (TraverseFocusPath(std::move(focusedChild), childLC, dc, settings))
			{
				return true;
			}
		}

		return settings.visitor(std::move(wnd));
	}

	return false;
}

static std::shared_ptr<Window> NavigateMostDescendantFocus(TextureManager &texman, std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc, Navigate navigate, NavigationPhase phase)
{
	if (wnd->GetVisible() && wnd->GetEnabled(dc))
	{
		std::shared_ptr<Window> handledBy;
		if (auto focusedChild = wnd->GetFocus())
		{
			auto childRect = wnd->GetChildRect(texman, lc, dc, *focusedChild);
			LayoutContext childLC(*wnd, lc, *focusedChild, Size(childRect), dc);
			handledBy = NavigateMostDescendantFocus(texman, std::move(focusedChild), childLC, dc, navigate, phase);
		}

		if (handledBy)
		{
			return handledBy;
		}
		else if (auto navigationSink = wnd->GetNavigationSink(); navigationSink && navigationSink->CanNavigate(navigate, lc, dc))
		{
			navigationSink->OnNavigate(navigate, phase, lc, dc);
			return wnd;
		}
	}
	return nullptr;
}

static FRECT EnsureVisibleMostDescendantFocus(TextureManager &texman, std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc)
{
	if (auto focusedChild = wnd->GetFocus())
	{
		auto childRect = wnd->GetChildRect(texman, lc, dc, *focusedChild);
		LayoutContext childLC(*wnd, lc, *focusedChild, Size(childRect), dc);

		FRECT pxFocusRect = EnsureVisibleMostDescendantFocus(texman, focusedChild, childLC, dc);

		if (auto scrollSink = wnd->GetScrollSink())
		{
			scrollSink->EnsureVisible(texman, lc, dc, pxFocusRect);

			// Get updated childRect as it may have changed due to scroll offset change
			childRect = wnd->GetChildRect(texman, lc, dc, *focusedChild);
		}

		return RectOffset(pxFocusRect, Offset(childRect));
	}
	else
	{
		return MakeRectWH(lc.GetPixelSize());
	}
}

static Navigate GetNavigateAction(Key key, bool alt, bool shift)
{
	switch (key)
	{
	case Key::Enter:
	case Key::NumEnter:
	case Key::Space:
	case Key::GamepadA:
		return Navigate::Enter;

	case Key::Backspace:
	case Key::Escape:
	case Key::GamepadB:
		return Navigate::Back;

	case Key::Tab:
		return shift ? Navigate::Prev : Navigate::Next;

	case Key::Up:
	case Key::GamepadLeftThumbstickUp:
	case Key::GamepadDPadUp:
		return Navigate::Up;

	case Key::Down:
	case Key::GamepadLeftThumbstickDown:
	case Key::GamepadDPadDown:
		return Navigate::Down;

	case Key::Left:
		if (alt)
		{
			return Navigate::Back;
		}
		// fallthrough
	case Key::GamepadLeftThumbstickLeft:
	case Key::GamepadDPadLeft:
		return Navigate::Left;

	case Key::Right:
	case Key::GamepadLeftThumbstickRight:
	case Key::GamepadDPadRight:
		return Navigate::Right;

	case Key::Home:
		return Navigate::Begin;

	case Key::End:
		return Navigate::End;
	}

	return Navigate::None;
}

bool InputContext::ProcessKeys(TextureManager &texman, std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc, Msg msg, Key key, float time)
{
	_lastKeyTime = time;

	bool handled = false;

	switch (msg)
	{
	case Msg::KeyReleased:
		break;
	case Msg::KeyPressed:
		handled = TraverseFocusPath(wnd, lc, dc, TraverseFocusPathSettings {
			texman,
			*this,
			[key, this](std::shared_ptr<Window> wnd) // visitor
			{
				KeyboardSink *keyboardSink = wnd->GetKeyboardSink();
				return keyboardSink ? keyboardSink->OnKeyPressed(*this, key) : false;
			}
		});
		break;
	default:
		assert(false);
	}

	if (!handled)
	{
		Navigate navigate = GetNavigateAction(key,
			GetInput().IsKeyPressed(Key::LeftAlt) || GetInput().IsKeyPressed(Key::RightAlt),
			GetInput().IsKeyPressed(Key::LeftShift) || GetInput().IsKeyPressed(Key::RightShift));
		if (Msg::KeyReleased == msg)
		{
			if (auto wnd = _navigationSubjects[navigate].lock())
			{
				// FIXME: reconstruct the actual LC
				wnd->GetNavigationSink()->OnNavigate(navigate, NavigationPhase::Completed, lc, dc);
				_navigationSubjects[navigate].reset();
				handled = true;
			}
		}
		else
		{
			auto handledBy = NavigateMostDescendantFocus(texman, wnd, lc, dc, navigate, NavigationPhase::Started);
			handled = !!handledBy;
			_navigationSubjects[navigate] = std::move(handledBy);

			if (handled)
			{
				EnsureVisibleMostDescendantFocus(texman, wnd, lc, dc);
			}
		}
	}

	return handled;
}

bool InputContext::ProcessText(TextureManager &texman, std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc, int c)
{
	return TraverseFocusPath(wnd, lc, dc, TraverseFocusPathSettings {
		texman,
		*this,
		[c](std::shared_ptr<Window> wnd) // visitor
		{
			TextSink *textSink = wnd->GetTextSink();
			return textSink ? textSink->OnChar(c) : false;
		}
	});
}

bool InputContext::ProcessSystemNavigationBack(TextureManager &texman, std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc)
{
	auto startedHandledBy = NavigateMostDescendantFocus(texman, wnd, lc, dc, Navigate::Back, NavigationPhase::Started);
	if (startedHandledBy)
	{
		// FIXME: reconstruct the actual LC
		startedHandledBy->GetNavigationSink()->OnNavigate(Navigate::Back, NavigationPhase::Completed, lc, dc);
	}
	return !!startedHandledBy;
}
