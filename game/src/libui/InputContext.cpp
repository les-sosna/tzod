#include "inc/ui/InputContext.h"
#include "inc/ui/Window.h"

using namespace UI;

InputContext::InputContext(IInput &input, IClipboard &clipboard)
	: _input(input)
	, _clipboard(clipboard)
	, _captureCountSystem(0)
	, _isAppActive(true)
#ifndef NDEBUG
	, _lastPointerLocation()
#endif
{
}

std::shared_ptr<Window> InputContext::GetCapture(unsigned int pointerID) const
{
	auto it = _pointerCaptures.find(pointerID);
	return _pointerCaptures.end() != it ? it->second.captureWnd.lock() : nullptr;
}

bool InputContext::HasCapturedPointers(Window *wnd) const
{
	for (auto &capture : _pointerCaptures)
	{
		if (capture.second.captureWnd.lock().get() == wnd)
		{
			return true;
		}
	}
	return false;
}

void InputContext::SetCapture(unsigned int pointerID, std::shared_ptr<Window> wnd)
{
	if (wnd)
	{
		if (!_pointerCaptures[pointerID].captureWnd.expired())
		{
			assert(_pointerCaptures[pointerID].captureWnd.lock() == wnd);
			assert(_pointerCaptures[pointerID].captureCount != 0);
		}
		else
		{
			assert(0 == _pointerCaptures[pointerID].captureCount);
			_pointerCaptures[pointerID].captureWnd = wnd;
		}
		_pointerCaptures[pointerID].captureCount++;
	}
	else
	{
		assert(!_pointerCaptures[pointerID].captureWnd.expired());
		assert(0 != _pointerCaptures[pointerID].captureCount);
		if (0 == --_pointerCaptures[pointerID].captureCount)
		{
			_pointerCaptures[pointerID].captureWnd.reset();
		}
	}
}

void InputContext::ResetWindow(Window &wnd)
{
	if (_hotTrackWnd.lock().get() == &wnd)
	{
		wnd.OnMouseLeave();
		_hotTrackWnd.reset();
	}

	_pointerCaptures.clear();
}

bool InputContext::ProcessPointerInternal(
	vec2d size,
	std::shared_ptr<Window> wnd,
	float x,
	float y,
	float z,
	Msg msg,
	int buttons,
	PointerType pointerType,
	unsigned int pointerID,
	bool topMostPass,
	bool insideTopMost)
{
	insideTopMost |= wnd->GetTopMost();

	if (!wnd->GetEnabled() || !wnd->GetVisible() || (insideTopMost && !topMostPass))
		return false;

	bool pointerInside = (x >= 0 && x < size.x && y >= 0 && y < size.y);

	if ((pointerInside || !wnd->GetClipChildren()) && GetCapture(pointerID) != wnd)
	{
		// route message to each child in reverse order until someone process it
		for (auto it = wnd->_children.rbegin(); it != wnd->_children.rend(); ++it)
		{
			auto &child = *it;
			FRECT rect = wnd->GetChildRect(size, *child);
			if (ProcessPointerInternal(Size(rect),
				child,
				x - rect.left,
				y - rect.top,
				z,
				msg,
				buttons,
				pointerType,
				pointerID,
				topMostPass,
				insideTopMost))
			{
				switch (msg)
				{
				case Msg::PointerDown:
				case Msg::TAP:
					if (child->GetEnabled() && child->GetVisible() && child->GetNeedsFocus())
					{
						wnd->SetFocus(child);
					}
				default:
					break;
				}

				return true;
			}
		}
	}

	if (insideTopMost == topMostPass && (pointerInside || GetCapture(pointerID) == wnd))
	{
		// window is captured or the pointer is inside the window

		bool msgProcessed = false;
		switch (msg)
		{
		case Msg::PointerDown:
			msgProcessed = wnd->OnPointerDown(*this, x, y, buttons, pointerType, pointerID);
			break;
		case Msg::PointerUp:
		case Msg::PointerCancel:
			msgProcessed = wnd->OnPointerUp(*this, x, y, buttons, pointerType, pointerID);
			break;
		case Msg::PointerMove:
			msgProcessed = wnd->OnPointerMove(*this, x, y, pointerType, pointerID);
			break;
		case Msg::MOUSEWHEEL:
			msgProcessed = wnd->OnMouseWheel(x, y, z);
			break;
		case Msg::TAP:
			msgProcessed = wnd->OnTap(*this, x, y);
			break;
		default:
			assert(false);
		}

		if (msgProcessed)
		{
			if (wnd != _hotTrackWnd.lock())
			{
				if (auto hotTrackWnd = _hotTrackWnd.lock())
					hotTrackWnd->OnMouseLeave();

				if (wnd->GetVisible() && wnd->GetEnabled())
				{
					_hotTrackWnd = wnd;
					wnd->OnMouseEnter(x, y);
				}
			}
		}

		return msgProcessed;
	}

	return false;
}

static FRECT GetGlobalWindowRect(const vec2d &rootSize, const Window &wnd)
{
	if (Window *parent = wnd.GetParent())
	{
		FRECT parentRect = GetGlobalWindowRect(rootSize, *parent);
		FRECT childRect = parent->GetChildRect(Size(parentRect), wnd);
		childRect.left += parentRect.left;
		childRect.right += parentRect.left;
		childRect.top += parentRect.top;
		childRect.bottom += parentRect.top;
		return childRect;
	}
	else
	{
		return FRECT{ 0, 0, rootSize.x, rootSize.y };
	}
}

bool InputContext::ProcessPointer(std::shared_ptr<Window> wnd, vec2d size, float x, float y, float z, Msg msg, int button, PointerType pointerType, unsigned int pointerID)
{
#ifndef NDEBUG
	_lastPointerLocation[pointerID] = vec2d(x, y);
#endif

	if (auto captured = GetCapture(pointerID))
	{
		FRECT capturetRect = GetGlobalWindowRect(size, *captured);
		vec2d capturedSize = Size(capturetRect);

		x -= capturetRect.left;
		y -= capturetRect.top;

		if (ProcessPointerInternal(capturedSize, captured, x, y, z, msg, button, pointerType, pointerID, true) ||
			ProcessPointerInternal(capturedSize, captured, x, y, z, msg, button, pointerType, pointerID, false))
		{
			return true;
		}
	}
	else
	{
		// handle all children of the desktop recursively; offer to topmost windows first
		if (ProcessPointerInternal(size, wnd, x, y, z, msg, button, pointerType, pointerID, true) ||
			ProcessPointerInternal(size, wnd, x, y, z, msg, button, pointerType, pointerID, false))
		{
			return true;
		}
	}
	if (auto hotTrackWnd = _hotTrackWnd.lock())
	{
		hotTrackWnd->OnMouseLeave();
		_hotTrackWnd.reset();
	}
	return false;
}

bool InputContext::ProcessKeyPressedRecursive(std::shared_ptr<Window> wnd, Key key)
{
	if (auto focus = wnd->GetFocus())
	{
		if (ProcessKeyPressedRecursive(std::move(focus), key))
		{
			return true;
		}
	}
	return wnd->OnKeyPressed(*this, key);
}

bool InputContext::ProcessKeys(std::shared_ptr<Window> wnd, Msg msg, Key key)
{
	switch (msg)
	{
	case Msg::KEYUP:
		break;
	case Msg::KEYDOWN:
		return ProcessKeyPressedRecursive(wnd, key);
	default:
		assert(false);
	}

	return false;
}

bool InputContext::ProcessCharRecursive(std::shared_ptr<Window> wnd, int c)
{
	if (auto focus = wnd->GetFocus())
	{
		if (ProcessCharRecursive(std::move(focus), c))
		{
			return true;
		}
	}
	return wnd->OnChar(c);
}

bool InputContext::ProcessText(std::shared_ptr<Window> wnd, int c)
{
	return ProcessCharRecursive(wnd, c);
}
