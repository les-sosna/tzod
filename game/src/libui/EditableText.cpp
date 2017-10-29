#include "inc/ui/EditableText.h"
#include "inc/ui/Clipboard.h"
#include "inc/ui/DataSource.h"
#include "inc/ui/InputContext.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/Keys.h"
#include "inc/ui/UIInput.h"
#include <video/TextureManager.h>
#include <video/RenderContext.h>

#include <algorithm>
#include <cstring>
#include <sstream>

using namespace UI;

EditableText::EditableText()
{
	SetClipChildren(true);
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

struct membuf: std::streambuf
{
	membuf(std::string_view s)
	{
		char *p = const_cast<char*>(s.data());
		setg(p, p, p + s.size());
	}
};
struct imemstream: virtual membuf, std::istream
{
	imemstream(std::string_view s)
		: membuf(s)
		, std::istream(static_cast<std::streambuf*>(this))
	{}
};

int EditableText::GetInt() const
{
	imemstream tmp(GetText());
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
	imemstream tmp(GetText());
	float result = 0;
	tmp >> result;
	return result;
}

void EditableText::SetSel(int begin, int end)
{
	assert(begin >= -1 && end >= -1);

	_selStart = begin <= GetTextLength() ? begin : -1;
	_selEnd = end <= GetTextLength() ? end : -1;
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

void EditableText::Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const
{
	size_t font = _font.GetTextureId(texman);
	float pxCharWidth = ToPx(texman.GetCharWidth(font), lc);

	// selection
	if (GetSelLength() && ic.GetFocused())
	{
		FRECT rt;
		rt.left = 1 + GetSelMin() * pxCharWidth;
		rt.top = 0;
		rt.right = rt.left + pxCharWidth * GetSelLength() - 1;
		rt.bottom = rt.top + lc.GetPixelSize().y;
		rc.DrawSprite(rt, _selection.GetTextureId(texman), 0xffffffff, 0);
	}

	// text
	SpriteColor c = lc.GetEnabledCombined() ? 0xffffffff : 0xaaaaaaaa;
	rc.DrawBitmapText(vec2d{ 0, 1 }, lc.GetScale(), font, c, GetText().substr(0, GetSelMin()));
	rc.DrawBitmapText(vec2d{ GetSelMin() * pxCharWidth, 1 }, lc.GetScale(), font, 0xffff0000, GetText().substr(GetSelMin(), GetSelLength()));
	rc.DrawBitmapText(vec2d{ GetSelMax() * pxCharWidth, 1 }, lc.GetScale(), font, c, GetText().substr(GetSelMax()));

	// cursor
	if (ic.GetFocused() && fmodf(time - ic.GetLastKeyTime(), 1.0f) < 0.5f)
	{
		FRECT rt = MakeRectWH(vec2d{ GetSelEnd() * pxCharWidth, 0 }, vec2d{ ToPx(texman.GetFrameWidth(_cursor.GetTextureId(texman), 0), lc), lc.GetPixelSize().y });
		rc.DrawSprite(rt, _cursor.GetTextureId(texman), 0xffffffff, 0);
	}
}

bool EditableText::OnChar(int c)
{
	if (isprint((unsigned char)c) && '\t' != c)
	{
		int start = GetSelMin();
		SetText(std::string(GetText().substr(0, start))
				.append(1, static_cast<std::string::value_type>(c))
				.append(GetText().substr(GetSelMax())));
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
			SetText(std::string(GetText().substr(0, GetSelMin())).append(GetText().substr(GetSelMax())));
			SetSel(GetSelMin(), GetSelMin());
			return true;
		}
		break;
	case Key::Delete:
		if (0 == GetSelLength() && GetSelEnd() < GetTextLength())
		{
			SetText(std::string(GetText().substr(0, GetSelStart())).append(
				GetText().substr(GetSelEnd() + 1, GetTextLength() - GetSelEnd() - 1)));
		}
		else
		{
			if (shift)
			{
				Copy(ic.GetClipboard());
			}
			SetText(std::string(GetText().substr(0, GetSelMin())).append(GetText().substr(GetSelMax())));
		}
		SetSel(GetSelMin(), GetSelMin());
		return true;
	case Key::Backspace:
		tmp = std::max(0, 0 == GetSelLength() ? GetSelStart() - 1 : GetSelMin());
		if (0 == GetSelLength() && GetSelStart() > 0)
		{
			SetText(std::string(GetText().substr(0, GetSelStart() - 1)).append(GetText().substr(GetSelEnd(), GetTextLength() - GetSelEnd())));
		}
		else
		{
			SetText(std::string(GetText().substr(0, GetSelMin())).append(GetText().substr(GetSelMax())));
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

bool EditableText::OnPointerDown(InputContext &ic, LayoutContext &lc, TextureManager &texman, PointerInfo pi, int button)
{
	if (pi.type == PointerType::Mouse && 1 == button && !ic.HasCapturedPointers(this))
	{
		int sel = HitTest(texman, pi.position, lc.GetScale());
		SetSel(sel, sel);
		return true;
	}
	return false;
}

void EditableText::OnPointerMove(InputContext &ic, LayoutContext &lc, TextureManager &texman, PointerInfo pi, bool captured)
{
	if (captured)
	{
		int sel = HitTest(texman, pi.position, lc.GetScale());
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

std::string_view EditableText::GetText() const
{
	return _text;
}

void EditableText::SetText(std::string text)
{
	_text = std::move(text);
	SetSel(_selStart, _selEnd);
	if (eventChange)
		eventChange();
}

int EditableText::HitTest(TextureManager &texman, vec2d px, float scale) const
{
	float pxCharWidth = std::floor((texman.GetFrameWidth(_font.GetTextureId(texman), 0) - 1) * scale);
	return std::min(GetTextLength(), std::max(0, int(px.x / pxCharWidth)));
}

void EditableText::Paste(const IClipboard &clipboard)
{
	auto data = clipboard.GetClipboardText();
	if (!data.empty())
	{
		std::ostringstream buf;
		buf << GetText().substr(0, GetSelMin());
		buf << data;
		buf << GetText().substr(GetSelMax(), GetText().length() - GetSelMax());
		SetText(buf.str());
		int sel = GetSelMin() + static_cast<int>(data.size());
		SetSel(sel, sel);
	}
}

void EditableText::Copy(IClipboard &clipboard) const
{
	auto sel = GetText().substr(GetSelMin(), GetSelLength());
	if (!sel.empty())
	{
		clipboard.SetClipboardText(std::string(sel));
	}
}

vec2d EditableText::GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const
{
	return ToPx(vec2d{ texman.GetCharWidth(_font.GetTextureId(texman)) * GetTextLength() + 20, texman.GetCharHeight(_font.GetTextureId(texman)) }, scale);
}
