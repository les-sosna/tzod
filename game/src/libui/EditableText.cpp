#include "inc/ui/EditableText.h"
#include "inc/ui/DataSource.h"
#include "inc/ui/InputContext.h"
#include "inc/ui/LayoutContext.h"
#include <plat/Input.h>
#include <plat/Keys.h>
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

float EditableText::GetCursorWidth(TextureManager &texman, float scale) const
{
	return ToPx(texman.GetFrameWidth(_cursor.GetTextureId(texman), 0), scale);
}

FRECT EditableText::GetCursorRect(TextureManager &texman, const LayoutContext &lc) const
{
	size_t font = _font.GetTextureId(texman);
	float pxCharWidth = ToPx(texman.GetCharWidth(font), lc);
	return MakeRectWH(vec2d{ GetSelEnd() * pxCharWidth, 0 }, vec2d{ GetCursorWidth(texman, lc.GetScaleCombined()), lc.GetPixelSize().y });
}

WindowLayout EditableText::GetChildLayout(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	assert(_fakeCursorPlaceholder.get() == &child);
	return WindowLayout{ GetCursorRect(texman, lc), 1, true };
}

std::shared_ptr<Window> EditableText::GetFocus(const std::shared_ptr<const Window>& owner) const
{
	return _fakeCursorPlaceholder;
}

Window* EditableText::GetFocus() const
{
	return _fakeCursorPlaceholder.get();
}

void EditableText::Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, float time, bool hovered) const
{
	size_t font = _font.GetTextureId(texman);
	float pxCharWidth = ToPx(texman.GetCharWidth(font), lc);

	// selection
	if (GetSelLength() && lc.GetFocusedCombined())
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
	rc.DrawBitmapText(vec2d{ 0, 1 }, lc.GetScaleCombined(), font, c, GetText().substr(0, GetSelMin()));
	rc.DrawBitmapText(vec2d{ GetSelMin() * pxCharWidth, 1 }, lc.GetScaleCombined(), font, 0xffff0000, GetText().substr(GetSelMin(), GetSelLength()));
	rc.DrawBitmapText(vec2d{ GetSelMax() * pxCharWidth, 1 }, lc.GetScaleCombined(), font, c, GetText().substr(GetSelMax()));

	// cursor
	if (lc.GetFocusedCombined() && fmodf(time - ic.GetLastKeyTime(), 1.0f) < 0.5f)
	{
		rc.DrawSprite(GetCursorRect(texman, lc), _cursor.GetTextureId(texman), 0xffffffff, 0);
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

bool EditableText::OnKeyPressed(const InputContext &ic, Plat::Key key)
{
	bool shift = ic.GetInput().IsKeyPressed(Plat::Key::LeftShift) ||
		ic.GetInput().IsKeyPressed(Plat::Key::RightShift);
	int tmp;
	switch (key)
	{
	case Plat::Key::Delete:
		if (0 == GetSelLength() && GetSelEnd() < GetTextLength())
		{
			SetText(std::string(GetText().substr(0, GetSelStart())).append(
				GetText().substr(GetSelEnd() + 1, GetTextLength() - GetSelEnd() - 1)));
		}
		else
		{
			SetText(std::string(GetText().substr(0, GetSelMin())).append(GetText().substr(GetSelMax())));
		}
		SetSel(GetSelMin(), GetSelMin());
		return true;
	case Plat::Key::Backspace:
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
	case Plat::Key::Left:
		if (shift)
		{
			tmp = std::max(0, GetSelEnd() - 1);
			SetSel(GetSelStart(), tmp);
		}
		else
		{
			tmp = std::max(0, GetSelMin() - (GetSelLength() > 0 ? 0 : 1));
			SetSel(tmp, tmp);
		}
		return true;
	case Plat::Key::Right:
		if (shift)
		{
			tmp = std::min(GetTextLength(), GetSelEnd() + 1);
			SetSel(GetSelStart(), tmp);
		}
		else
		{
			tmp = std::min(GetTextLength(), GetSelMax() + (GetSelLength() > 0 ? 0 : 1));
			SetSel(tmp, tmp);
		}
		return true;
	case Plat::Key::Home:
		if (shift)
		{
			SetSel(GetSelStart(), 0);
		}
		else
		{
			SetSel(0, 0);
		}
		return true;
	case Plat::Key::End:
		if (shift)
		{
			SetSel(GetSelStart(), -1);
		}
		else
		{
			SetSel(-1, -1);
		}
		return true;
	case Plat::Key::Space:
		return true;

	default:
		break;
	}
	return false;
}

bool EditableText::OnPointerDown(const InputContext &ic, const LayoutContext &lc, TextureManager &texman, PointerInfo pi, int button)
{
	if (pi.type == Plat::PointerType::Mouse && 1 == button && !ic.HasCapturedPointers(this))
	{
		int sel = HitTest(texman, pi.position, lc.GetScaleCombined());
		SetSel(sel, sel);
		return true;
	}
	return false;
}

void EditableText::OnPointerMove(const InputContext &ic, const LayoutContext &lc, TextureManager &texman, PointerInfo pi, bool captured)
{
	if (captured)
	{
		int sel = HitTest(texman, pi.position, lc.GetScaleCombined());
		SetSel(GetSelStart(), sel);
	}
}

void EditableText::OnTap(const InputContext &ic, const LayoutContext &lc, TextureManager &texman, vec2d pointerPosition)
{
	if (!ic.HasCapturedPointers(this))
	{
		int sel = HitTest(texman, pointerPosition, lc.GetScaleCombined());
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

void EditableText::OnPaste(std::string_view text)
{
	std::ostringstream buf;
	buf << GetText().substr(0, GetSelMin());
	buf << text;
	buf << GetText().substr(GetSelMax(), GetText().length() - GetSelMax());
	SetText(buf.str());
	int sel = GetSelMin() + static_cast<int>(text.size());
	SetSel(sel, sel);
}

std::string_view EditableText::OnCopy() const
{
	return GetText().substr(GetSelMin(), GetSelLength());
}

std::string EditableText::OnCut()
{
	auto result = _text.substr(GetSelMin(), GetSelLength());
	SetText(std::string(GetText().substr(0, GetSelMin())).append(GetText().substr(GetSelMax())));
	SetSel(GetSelMin(), GetSelMin());
	return result;
}

vec2d EditableText::GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const
{
	vec2d pxActualTextSize = ToPx(vec2d{ texman.GetCharWidth(_font.GetTextureId(texman)) * GetTextLength(), texman.GetCharHeight(_font.GetTextureId(texman)) }, scale);
	return pxActualTextSize + vec2d{ GetCursorWidth(texman, scale), 0 };
}
