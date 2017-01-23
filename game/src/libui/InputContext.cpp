#include "inc/ui/InputContext.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/StateContext.h"
#include "inc/ui/UIInput.h"
#include "inc/ui/Window.h"

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
	const StateContext &sc,
	const InputContext &ic,
	bool insideTopMost)
{
	SinkType *sink = nullptr;

	vec2d pxPointerPosition = search.pxGlobalPointerPosition - lc.GetPixelOffset();
	bool pointerInside = PtInFRect(MakeRectWH(lc.GetPixelSize()), pxPointerPosition);

	if (pointerInside || !wnd->GetClipChildren())
	{
		auto stateGen = wnd->GetStateGen();
		StateContext childSC;
		if (stateGen)
		{
			childSC = sc;
			stateGen->PushState(childSC, lc, ic);
		}

		auto &children = wnd->GetChildren();
		for (auto it = children.rbegin(); it != children.rend() && !sink; ++it)
		{
			auto &child = *it;
			if (child->GetEnabled(stateGen ? childSC : sc) && child->GetVisible())
			{
				// early skip topmost subtree
				bool childInsideTopMost = insideTopMost || child->GetTopMost();
				if (!childInsideTopMost || search.topMostPass)
				{
					LayoutContext childLC(search.texman, *wnd, lc, sc, *child, stateGen ? childSC : sc);
					sink = FindAreaSink<SinkType>(search, child, childLC, stateGen ? childSC : sc, ic, childInsideTopMost);
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

static LayoutContext RestoreLayoutContext(LayoutContext lc, StateContext sc, const InputContext &ic, TextureManager &texman, const std::vector<std::shared_ptr<Window>> &path)
{
	for (size_t i = path.size() - 1; i > 0; i--)
	{
		auto &child = path[i - 1];
		auto &parent = path[i];

		StateContext childSC;
		auto stateGen = parent->GetStateGen();
		if (stateGen)
		{
			childSC = sc;
			stateGen->PushState(childSC, lc, ic);
		}

		lc = LayoutContext(texman, *parent, lc, sc, *child, stateGen ? childSC : sc);
	}
	return lc;
}

bool InputContext::ProcessPointer(
	TextureManager &texman,
	std::shared_ptr<Window> wnd,
	const LayoutContext &lc,
	const StateContext &sc,
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
		return ProcessScroll(texman, wnd, lc, sc, pxPointerPosition, pxPointerOffset);
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
			AreaSinkSearch search{ texman, pxPointerPosition, topMostPass };
			pointerSink = FindAreaSink<PointerSink>(search, wnd, lc, sc, *this, wnd->GetTopMost());
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

		if ((Msg::PointerDown == msg || Msg::TAP == msg) && NeedsFocus(target.get()))
			PropagateFocus(sinkPath);

		LayoutContext childLC = RestoreLayoutContext(lc, sc, *this, texman, sinkPath);

		pxPointerPosition -= childLC.GetPixelOffset();

		switch (msg)
		{
		case Msg::PointerDown:
			if (pointerSink->OnPointerDown(*this, childLC, texman, pxPointerPosition, button, pointerType, pointerID))
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
				pointerSink->OnPointerUp(*this, childLC, texman, pxPointerPosition, button, pointerType, pointerID);
			}
			break;
		case Msg::PointerMove:
			pointerSink->OnPointerMove(*this, childLC, texman, pxPointerPosition, pointerType, pointerID, isPointerCaptured);
			break;
		case Msg::TAP:
			pointerSink->OnTap(*this, childLC, texman, pxPointerPosition);
			break;
		default:
			assert(false);
		}
	}

	return !!pointerSink;
}

bool InputContext::ProcessScroll(TextureManager &texman, std::shared_ptr<Window> wnd, const LayoutContext &lc, const StateContext &sc, vec2d pxPointerPosition, vec2d offset)
{
	ScrollSink *scrollSink = nullptr;
	std::vector<std::shared_ptr<Window>> sinkPath;

	// look for topmost windows first
	for (bool topMostPass : {true, false})
	{
		AreaSinkSearch search{ texman, pxPointerPosition, topMostPass };
		scrollSink = FindAreaSink<ScrollSink>(search, wnd, lc, sc, *this, wnd->GetTopMost());
		if (scrollSink)
		{
			sinkPath = std::move(search.outSinkPath);
			break;
		}
	}

	if (scrollSink)
	{
		LayoutContext childLC = RestoreLayoutContext(lc, sc, *this, texman, sinkPath);


		pxPointerPosition -= childLC.GetPixelOffset();

		scrollSink->OnScroll(texman, *this, childLC, sc, pxPointerPosition, offset);
		return true;
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
