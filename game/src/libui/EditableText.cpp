#include "inc/ui/EditableText.h"
#include "inc/ui/Clipboard.h"
#include "inc/ui/DataSource.h"
#include "inc/ui/InputContext.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/Keys.h"
#include "inc/ui/UIInput.h"
#include <video/TextureManager.h>
#include <video/RenderContext.h>

#include <algorithm>
#include <cstring>
#include <sstream>

using namespace UI;

EditableText::EditableText(LayoutManager &manager, TextureManager &texman)
	: Managerful(manager)
	, _selStart(-1)
	, _selEnd(-1)
	, _font(texman.FindSprite("font_small"))
	, _cursor(texman.FindSprite("ui/editcursor"))
	, _selection(texman.FindSprite("ui/editsel"))
{
	SetClipChildren(true);
	SetSel(0, 0);
}

int EditableText::GetTextLength() const
{
	return (int)GetText().length();
}

void EditableText::SetInt(int value)
{
	std::ostringstream tmp;
	tmp << value;
	SetText(tmp.str());
}

int EditableText::GetInt() const
{
	std::istringstream tmp(GetText());
	int result = 0;
	tmp >> result;
	return result;
}

void EditableText::SetFloat(float value)
{
	std::ostringstream tmp;
	tmp << value;
	SetText(tmp.str());
}

float EditableText::GetFloat() const
{
	std::istringstream tmp(GetText());
	float result = 0;
	tmp >> result;
	return result;
}

void EditableText::SetSel(int begin, int end)
{
	assert(begin >= -1 && end >= -1);

	_selStart = begin <= GetTextLength() ? begin : -1;
	_selEnd = end <= GetTextLength() ? end : -1;
	_lastCursortime = GetManager().GetTime();
}

int EditableText::GetSelStart() const
{
	return (unsigned)_selStart <= (unsigned)GetTextLength() ? _selStart : GetTextLength();
}

int EditableText::GetSelEnd() const
{
	return (unsigned)_selEnd <= (unsigned)GetTextLength() ? _selEnd : GetTextLength();
}

int EditableText::GetSelLength() const
{
	return abs(GetSelStart() - GetSelEnd());
}

int EditableText::GetSelMin() const
{
	return std::min(GetSelStart(), GetSelEnd());
}

int EditableText::GetSelMax() const
{
	return std::max(GetSelStart(), GetSelEnd());
}

void EditableText::Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman) const
{
	float pxCharWidth = ToPx(texman.GetCharWidth(_font), lc);

	// selection
	if (GetSelLength() && ic.GetFocused())
	{
		FRECT rt;
		rt.left = 1 + GetSelMin() * pxCharWidth;
		rt.top = 0;
		rt.right = rt.left + pxCharWidth * GetSelLength() - 1;
		rt.bottom = rt.top + lc.GetPixelSize().y;
		rc.DrawSprite(rt, _selection, 0xffffffff, 0);
	}

	// text
	SpriteColor c = lc.GetEnabledCombined() ? 0xffffffff : 0xaaaaaaaa;
	rc.DrawBitmapText(vec2d{ 0, 1 }, lc.GetScale(), _font, c, GetText().substr(0, GetSelMin()));
	rc.DrawBitmapText(vec2d{ GetSelMin() * pxCharWidth, 1 }, lc.GetScale(), _font, 0xffff0000, GetText().substr(GetSelMin(), GetSelLength()));
	rc.DrawBitmapText(vec2d{ GetSelMax() * pxCharWidth, 1 }, lc.GetScale(), _font, c, GetText().substr(GetSelMax()));

	float time = GetManager().GetTime() - _lastCursortime;

	// cursor
	if (ic.GetFocused() && fmodf(time, 1.0f) < 0.5f)
	{
		FRECT rt = MakeRectWH(vec2d{ GetSelEnd() * pxCharWidth, 0 }, vec2d{ ToPx(texman.GetFrameWidth(_cursor, 0), lc), lc.GetPixelSize().y });
		rc.DrawSprite(rt, _cursor, 0xffffffff, 0);
	}
}

bool EditableText::OnChar(int c)
{
	if (isprint((unsigned char)c) && '\t' != c)
	{
		int start = GetSelMin();
		SetText(GetText().substr(0, start) + (std::string::value_type) c + GetText().substr(GetSelMax()));
		SetSel(start + 1, start + 1);
		return true;
	}
	return false;
}

bool EditableText::OnKeyPressed(InputContext &ic, Key key)
{
	bool shift = ic.GetInput().IsKeyPressed(Key::LeftShift) ||
		ic.GetInput().IsKeyPressed(Key::RightShift);
	bool control = ic.GetInput().IsKeyPressed(Key::LeftCtrl) ||
		ic.GetInput().IsKeyPressed(Key::RightCtrl);
	int tmp;
	switch (key)
	{
	case Key::Insert:
		if (shift)
		{
			Paste(ic.GetClipboard());
			return true;
		}
		else if (control)
		{
			Copy(ic.GetClipboard());
			return true;
		}
		break;
	case Key::V:
		if (control)
		{
			Paste(ic.GetClipboard());
			return true;
		}
		break;
	case Key::C:
		if (control)
		{
			Copy(ic.GetClipboard());
			return true;
		}
		break;
	case Key::X:
		if (0 != GetSelLength() && control)
		{
			Copy(ic.GetClipboard());
			SetText(GetText().substr(0, GetSelMin()) + GetText().substr(GetSelMax()));
			SetSel(GetSelMin(), GetSelMin());
			return true;
		}
		break;
	case Key::Delete:
		if (0 == GetSelLength() && GetSelEnd() < GetTextLength())
		{
			SetText(GetText().substr(0, GetSelStart())
				+ GetText().substr(GetSelEnd() + 1, GetTextLength() - GetSelEnd() - 1));
		}
		else
		{
			if (shift)
			{
				Copy(ic.GetClipboard());
			}
			SetText(GetText().substr(0, GetSelMin()) + GetText().substr(GetSelMax()));
		}
		SetSel(GetSelMin(), GetSelMin());
		return true;
	case Key::Backspace:
		tmp = std::max(0, 0 == GetSelLength() ? GetSelStart() - 1 : GetSelMin());
		if (0 == GetSelLength() && GetSelStart() > 0)
		{
			SetText(GetText().substr(0, GetSelStart() - 1) + GetText().substr(GetSelEnd(), GetTextLength() - GetSelEnd()));
		}
		else
		{
			SetText(GetText().substr(0, GetSelMin()) + GetText().substr(GetSelMax()));
		}
		SetSel(tmp, tmp);
		return true;
	case Key::Left:
		if (shift)
		{
			tmp = std::max(0, GetSelEnd() - 1);
			SetSel(GetSelStart(), tmp);
		}
		else
		{
			tmp = std::max(0, GetSelMin() - 1);
			SetSel(tmp, tmp);
		}
		return true;
	case Key::Right:
		if (shift)
		{
			tmp = std::min(GetTextLength(), GetSelEnd() + 1);
			SetSel(GetSelStart(), tmp);
		}
		else
		{
			tmp = std::min(GetTextLength(), GetSelMax() + 1);
			SetSel(tmp, tmp);
		}
		return true;
	case Key::Home:
		if (shift)
		{
			SetSel(GetSelStart(), 0);
		}
		else
		{
			SetSel(0, 0);
		}
		return true;
	case Key::End:
		if (shift)
		{
			SetSel(GetSelStart(), -1);
		}
		else
		{
			SetSel(-1, -1);
		}
		return true;
	case Key::Space:
		return true;

	default:
		break;
	}
	return false;
}

bool EditableText::OnPointerDown(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID)
{
	if (pointerType == PointerType::Mouse && 1 == button && !ic.HasCapturedPointers(this))
	{
		int sel = HitTest(texman, pointerPosition, lc.GetScale());
		SetSel(sel, sel);
		return true;
	}
	return false;
}

void EditableText::OnPointerMove(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, PointerType pointerType, unsigned int pointerID, bool captured)
{
	if (captured)
	{
		int sel = HitTest(texman, pointerPosition, lc.GetScale());
		SetSel(GetSelStart(), sel);
	}
}

void EditableText::OnTap(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition)
{
	if (!ic.HasCapturedPointers(this))
	{
		int sel = HitTest(texman, pointerPosition, lc.GetScale());
		SetSel(sel, sel);
	}
}

KeyboardSink* EditableText::GetKeyboardSink()
{
	return this;
}

const std::string& EditableText::GetText() const
{
	return _text;
}

void EditableText::SetText(const std::string &text)
{
	_text.assign(text);
	SetSel(_selStart, _selEnd);
	if (eventChange)
		eventChange();
}

int EditableText::HitTest(TextureManager &texman, vec2d px, float scale) const
{
	float pxCharWidth = std::floor((texman.GetFrameWidth(_font, 0) - 1) * scale);
	return std::min(GetTextLength(), std::max(0, int(px.x / pxCharWidth)));
}

void EditableText::Paste(const IClipboard &clipboard)
{
	if (const char *data = clipboard.GetClipboardText())
	{
		std::ostringstream buf;
		buf << GetText().substr(0, GetSelMin());
		buf << data;
		buf << GetText().substr(GetSelMax(), GetText().length() - GetSelMax());
		SetText(buf.str());
		SetSel(GetSelMin() + std::strlen(data), GetSelMin() + std::strlen(data));
	}
}

void EditableText::Copy(IClipboard &clipboard) const
{
	std::string str = GetText().substr(GetSelMin(), GetSelLength());
	if (!str.empty())
	{
		clipboard.SetClipboardText(std::move(str));
	}
}

vec2d EditableText::GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const
{
	return ToPx(vec2d{ texman.GetCharWidth(_font) * GetTextLength() + 20, texman.GetCharHeight(_font) }, scale);
}
