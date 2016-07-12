#include "inc/ui/InputContext.h"
#include "inc/ui/Window.h"
#include "inc/ui/UIInput.h"

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

void InputContext::PushTransform(vec2d offset, bool focused, bool hovered)
{
	assert(!_transformStack.empty());
	_transformStack.push(InputStackFrame{
		_transformStack.top().offset + offset,
		_transformStack.top().focused && focused,
		_transformStack.top().hovered && hovered
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

bool InputContext::GetHovered() const
{
	return _transformStack.top().hovered;
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
	vec2d pxSize,
	vec2d pxPointerPosition,
	bool insideTopMost)
{
	PointerSink *pointerSink = nullptr;

	bool pointerInside = (pxPointerPosition.x >= 0 && pxPointerPosition.x < pxSize.x &&
		pxPointerPosition.y >= 0 && pxPointerPosition.y < pxSize.y);

	if (pointerInside || !wnd->GetClipChildren())
	{
		// route message to each child in reverse order until someone process it
		auto &children = wnd->GetChildren();
		for (auto it = children.rbegin(); it != children.rend() && !pointerSink; ++it)
		{
			auto &child = *it;
			if (child->GetEnabled() && child->GetVisible())
			{
				// early skip topmost subtree
				bool childInsideTopMost = insideTopMost || child->GetTopMost();
				if (!childInsideTopMost || search.topMostPass)
				{
					FRECT childRect = wnd->GetChildRect(pxSize, search.layoutScale, *child);
					vec2d childPointerPosition = pxPointerPosition - vec2d{ childRect.left, childRect.top };
					pointerSink = FindPointerSink(search,
						child,
						Size(childRect),
						childPointerPosition,
						childInsideTopMost);
				}
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

bool InputContext::ProcessPointer(
	std::shared_ptr<Window> wnd,
	float layoutScale,
	vec2d pxSize,
	vec2d pxPointerPosition,
	float z,
	Msg msg,
	int button,
	PointerType pointerType,
	unsigned int pointerID)
{
#ifndef NDEBUG
	_lastPointerLocation[pointerID] = pxPointerPosition;
#endif

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
			// look for topmost windows first
			PointerSinkSearch search{ layoutScale, topMostPass };
			pointerSink = FindPointerSink(search, wnd, pxSize, pxPointerPosition, wnd->GetTopMost());
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
		bool setFocus = (Msg::PointerDown == msg || Msg::TAP == msg) && NeedsFocus(target.get());

		for (size_t i = sinkPath.size() - 1; i > 0; i--)
		{
			auto &child = sinkPath[i - 1];
			auto &parent = sinkPath[i];

			if (setFocus)
				parent->SetFocus(child);

			FRECT childRect = parent->GetChildRect(pxSize, layoutScale, *child);
			pxPointerPosition -= {childRect.left, childRect.top};
			pxSize = Size(childRect);
		}

		switch (msg)
		{
		case Msg::PointerDown:
			if (pointerSink->OnPointerDown(*this, pxSize, pxPointerPosition, button, pointerType, pointerID))
			{
				_pointerCaptures[pointerID].capturePath = sinkPath;
			}
			break;
		case Msg::PointerUp:
		case Msg::PointerCancel:
			if (isPointerCaptured)
			{
				pointerSink->OnPointerUp(*this, pxSize, pxPointerPosition, button, pointerType, pointerID);
				_pointerCaptures.erase(pointerID);
			}
			break;
		case Msg::PointerMove:
			pointerSink->OnPointerMove(*this, pxSize, pxPointerPosition, pointerType, pointerID, isPointerCaptured);
			break;
		case Msg::MOUSEWHEEL:
			pointerSink->OnMouseWheel(*this, pxSize, pxPointerPosition, z);
			break;
		case Msg::TAP:
			pointerSink->OnTap(*this, pxSize, pxPointerPosition);
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
