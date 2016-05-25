#include "inc/ui/InputContext.h"
#include "inc/ui/Window.h"
#include "inc/ui/UIInput.h"

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
	_transformStack.emplace(0.f, 0.f);
}

void InputContext::PushTransform(vec2d offset)
{
	assert(!_transformStack.empty());
	_transformStack.push(_transformStack.top() + offset);
}

void InputContext::PopTransform()
{
	assert(_transformStack.size() > 1);
	_transformStack.pop();
}

vec2d InputContext::GetMousePos() const
{
	return _input.GetMousePos() - _transformStack.top();
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
		assert(wnd.GetPointerSink());
		wnd.GetPointerSink()->OnMouseLeave();
		_hotTrackWnd.reset();
	}

	_pointerCaptures.clear();
}

struct PointerSinkSearch
{
	bool topMostPass;
	std::shared_ptr<Window> captured;
	std::vector<std::shared_ptr<Window>> outSinkPath;
};

static PointerSink* FindPointerSink(
	PointerSinkSearch &search,
	std::shared_ptr<Window> wnd,
	vec2d size,
	vec2d pointerPosition,
	bool insideTopMost)
{
	if (insideTopMost && !search.topMostPass)
		return nullptr;

	PointerSink *pointerSink = nullptr;

	bool pointerInside = (pointerPosition.x >= 0 && pointerPosition.x < size.x &&
		pointerPosition.y >= 0 && pointerPosition.y < size.y);

	if ((pointerInside || !wnd->GetClipChildren()) && search.captured != wnd)
	{
		// route message to each child in reverse order until someone process it
		auto &children = wnd->GetChildren();
		for (auto it = children.rbegin(); it != children.rend() && !pointerSink; ++it)
		{
			auto &child = *it;
			if (child->GetEnabled() && child->GetVisible())
			{
				FRECT childRect = wnd->GetChildRect(size, *child);
				vec2d childPointerPosition = pointerPosition - vec2d(childRect.left, childRect.top);
				bool childInsideTopMost = insideTopMost || child->GetTopMost();
				pointerSink = FindPointerSink(search,
					child,
					Size(childRect),
					childPointerPosition,
					childInsideTopMost);
			}
		}
	}

	// if window is captured or the pointer is inside the window
	if (!pointerSink && insideTopMost == search.topMostPass && (pointerInside || search.captured == wnd))
		pointerSink = wnd->GetPointerSink();

	if (pointerSink)
		search.outSinkPath.push_back(wnd);

	return pointerSink;
}

bool InputContext::ProcessPointerInternal(
	vec2d size,
	std::shared_ptr<Window> wnd,
	vec2d pointerPosition,
	float z,
	Msg msg,
	int buttons,
	PointerType pointerType,
	unsigned int pointerID,
	bool topMostPass)
{
	PointerSinkSearch search{ topMostPass, GetCapture(pointerID) };
	PointerSink *pointerSink = FindPointerSink(search, wnd, size, pointerPosition, wnd->GetTopMost());
	if (pointerSink)
	{
		assert(search.outSinkPath.back() == wnd);

		auto &target = search.outSinkPath.front();
		bool setFocus = (Msg::PointerDown == msg || Msg::TAP == msg) && NeedsFocus(target.get());

		for (size_t i = search.outSinkPath.size() - 1; i > 0; i--)
		{
			auto &child = search.outSinkPath[i - 1];
			auto &parent = search.outSinkPath[i];

			if (setFocus)
				parent->SetFocus(child);

			FRECT childRect = parent->GetChildRect(size, *child);
			pointerPosition -= vec2d(childRect.left, childRect.top);
			size = Size(childRect);
		}

		switch (msg)
		{
		case Msg::PointerDown:
			pointerSink->OnPointerDown(*this, pointerPosition, buttons, pointerType, pointerID);
			break;
		case Msg::PointerUp:
		case Msg::PointerCancel:
			pointerSink->OnPointerUp(*this, pointerPosition, buttons, pointerType, pointerID);
			break;
		case Msg::PointerMove:
			pointerSink->OnPointerMove(*this, pointerPosition, pointerType, pointerID);
			break;
		case Msg::MOUSEWHEEL:
			pointerSink->OnMouseWheel(pointerPosition, z);
			break;
		case Msg::TAP:
			pointerSink->OnTap(*this, pointerPosition);
			break;
		default:
			assert(false);
		}

		if (target != _hotTrackWnd.lock())
		{
			if (auto hotTrackWnd = _hotTrackWnd.lock())
			{
				assert(hotTrackWnd->GetPointerSink());
				hotTrackWnd->GetPointerSink()->OnMouseLeave();
			}

			if (target->GetVisible() && target->GetEnabled())
			{
				_hotTrackWnd = target;
				pointerSink->OnMouseEnter(pointerPosition);
			}
		}
	}
	return !!pointerSink;
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

bool InputContext::ProcessPointer(std::shared_ptr<Window> wnd, vec2d size, vec2d pointerPosition, float z, Msg msg, int button, PointerType pointerType, unsigned int pointerID)
{
#ifndef NDEBUG
	_lastPointerLocation[pointerID] = pointerPosition;
#endif

	if (auto captured = GetCapture(pointerID))
	{
		FRECT capturetRect = GetGlobalWindowRect(size, *captured);
		vec2d capturedSize = Size(capturetRect);

		pointerPosition.x -= capturetRect.left;
		pointerPosition.y -= capturetRect.top;

		if (ProcessPointerInternal(capturedSize, captured, pointerPosition, z, msg, button, pointerType, pointerID, true) ||
			ProcessPointerInternal(capturedSize, captured, pointerPosition, z, msg, button, pointerType, pointerID, false))
		{
			return true;
		}
	}
	else
	{
		// handle all children of the desktop recursively; offer to topmost windows first
		if (ProcessPointerInternal(size, wnd, pointerPosition, z, msg, button, pointerType, pointerID, true) ||
			ProcessPointerInternal(size, wnd, pointerPosition, z, msg, button, pointerType, pointerID, false))
		{
			return true;
		}
	}
	if (auto hotTrackWnd = _hotTrackWnd.lock())
	{
		assert(hotTrackWnd->GetPointerSink());
		hotTrackWnd->GetPointerSink()->OnMouseLeave();
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

	if (KeyboardSink *keyboardSink = wnd->GetKeyboardSink())
	{
		return keyboardSink->OnKeyPressed(*this, key);
	}

	return false;
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

	if (TextSink *textSink = wnd->GetTextSink())
	{
		return textSink->OnChar(c);
	}

	return false;
}

bool InputContext::ProcessText(std::shared_ptr<Window> wnd, int c)
{
	return ProcessCharRecursive(wnd, c);
}
