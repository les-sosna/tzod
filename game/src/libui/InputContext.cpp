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
	_transformStack.emplace(InputStackFrame{vec2d{}, true});
}

void InputContext::PushTransform(vec2d offset, bool focused)
{
	assert(!_transformStack.empty());
	_transformStack.push(InputStackFrame{
		_transformStack.top().offset + offset,
		_transformStack.top().focused && focused
	});
}

void InputContext::PopTransform()
{
	assert(_transformStack.size() > 1);
	_transformStack.pop();
}

vec2d InputContext::GetMousePos() const
{
	return _input.GetMousePos() - _transformStack.top().offset;
}

bool InputContext::GetFocused() const
{
	return _transformStack.top().focused;
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

PointerSink* UI::FindPointerSink(
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

	if (pointerInside || !wnd->GetClipChildren())
	{
		// route message to each child in reverse order until someone process it
		auto &children = wnd->GetChildren();
		for (auto it = children.rbegin(); it != children.rend() && !pointerSink; ++it)
		{
			auto &child = *it;
			if (child->GetEnabled() && child->GetVisible())
			{
				FRECT childRect = wnd->GetChildRect(size, *child);
				vec2d childPointerPosition = pointerPosition - vec2d{ childRect.left, childRect.top };
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
	if (!pointerSink && insideTopMost == search.topMostPass && pointerInside)
		pointerSink = wnd->GetPointerSink();

	if (pointerSink)
		search.outSinkPath.push_back(wnd);

	return pointerSink;
}

bool InputContext::ProcessPointer(std::shared_ptr<Window> wnd, vec2d size, vec2d pointerPosition, float z, Msg msg, int button, PointerType pointerType, unsigned int pointerID)
{
#ifndef NDEBUG
	_lastPointerLocation[pointerID] = pointerPosition;
#endif

	PointerSink *pointerSink = nullptr;
	PointerSinkSearch search;

	const auto capturedIt = _pointerCaptures.find(pointerID);
	if (_pointerCaptures.end() != capturedIt)
	{
		pointerSink = capturedIt->second.capturePath.front()->GetPointerSink();
		assert(pointerSink);
		search.outSinkPath = capturedIt->second.capturePath;
	}
	else
	{
		for (int pass = 0; pass < 2 && !pointerSink; ++pass)
		{
			search.topMostPass = !pass; // look for topmost windows first
			pointerSink = FindPointerSink(search, wnd, size, pointerPosition, wnd->GetTopMost());
		}
	}

	if (pointerSink)
	{
		auto &target = search.outSinkPath.front();
		bool setFocus = (Msg::PointerDown == msg || Msg::TAP == msg) && NeedsFocus(target.get());

		for (size_t i = search.outSinkPath.size() - 1; i > 0; i--)
		{
			auto &child = search.outSinkPath[i - 1];
			auto &parent = search.outSinkPath[i];

			if (setFocus)
				parent->SetFocus(child);

			FRECT childRect = parent->GetChildRect(size, *child);
			pointerPosition -= {childRect.left, childRect.top};
			size = Size(childRect);
		}

		switch (msg)
		{
		case Msg::PointerDown:
			if (pointerSink->OnPointerDown(*this, size, pointerPosition, button, pointerType, pointerID))
			{
				_pointerCaptures[pointerID].capturePath = search.outSinkPath;
			}
			break;
		case Msg::PointerUp:
		case Msg::PointerCancel:
			if (_pointerCaptures.end() != capturedIt)
			{
				pointerSink->OnPointerUp(*this, size, pointerPosition, button, pointerType, pointerID);
				_pointerCaptures.erase(pointerID);
			}
			break;
		case Msg::PointerMove:
			pointerSink->OnPointerMove(*this, size, pointerPosition, pointerType, pointerID, _pointerCaptures.end() != capturedIt);
			break;
		case Msg::MOUSEWHEEL:
			pointerSink->OnMouseWheel(*this, size, pointerPosition, z);
			break;
		case Msg::TAP:
			pointerSink->OnTap(*this, size, pointerPosition);
			break;
		default:
			assert(false);
		}
	}

	return !!pointerSink;
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
