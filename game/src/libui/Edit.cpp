#include "inc/ui/Edit.h"
#include "inc/ui/Clipboard.h"
#include "inc/ui/InputContext.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/Keys.h"
#include "inc/ui/UIInput.h"
#include <video/TextureManager.h>
#include <video/DrawingContext.h>

#include <algorithm>
#include <cstring>
#include <sstream>

using namespace UI;

Edit::Edit(LayoutManager &manager, TextureManager &texman)
  : Window(manager)
  , _selStart(-1)
  , _selEnd(-1)
  , _offset(0)
  , _font(texman.FindSprite("font_small"))
  , _cursor(texman.FindSprite("ui/editcursor"))
  , _selection(texman.FindSprite("ui/editsel"))
{
	SetTexture(texman, "ui/edit", true);
	SetDrawBorder(true);
	SetClipChildren(true);
	SetSel(0, 0);
	Resize(GetWidth(), texman.GetCharHeight(_font) + 2);
}

int Edit::GetTextLength() const
{
	return (int) GetText().length();
}

void Edit::SetInt(int value)
{
	std::ostringstream tmp;
	tmp << value;
	SetText(GetManager().GetTextureManager(), tmp.str());
}

int Edit::GetInt() const
{
	std::istringstream tmp(GetText());
	int result = 0;
	tmp >> result;
	return result;
}

void Edit::SetFloat(float value)
{
	std::ostringstream tmp;
	tmp << value;
	SetText(GetManager().GetTextureManager(), tmp.str());
}

float Edit::GetFloat() const
{
	std::istringstream tmp(GetText());
	float result = 0;
	tmp >> result;
	return result;
}

void Edit::SetSel(int begin, int end)
{
	assert(begin >= -1 && end >= -1);

	_selStart = begin <= GetTextLength() ? begin : -1;
	_selEnd   = end <= GetTextLength() ? end : -1;
	_lastCursortime = GetManager().GetTime();

	float w = GetManager().GetTextureManager().GetFrameWidth(_font, 0) - 1;
	float cpos = GetSelEnd() * w;
	if( cpos - (float) (_offset * w) > GetWidth() - 10 || cpos - (float) (_offset * w) < 10 )
	{
		_offset = size_t(std::max<float>(0, cpos - GetWidth() * 0.5f) / w);
	}
}

int Edit::GetSelStart() const
{
	return (unsigned) _selStart <= (unsigned) GetTextLength() ? _selStart : GetTextLength();
}

int Edit::GetSelEnd() const
{
	return (unsigned) _selEnd <= (unsigned) GetTextLength() ? _selEnd : GetTextLength();
}

int Edit::GetSelLength() const
{
	return abs(GetSelStart() - GetSelEnd());
}

int Edit::GetSelMin() const
{
	return std::min(GetSelStart(), GetSelEnd());
}

int Edit::GetSelMax() const
{
	return std::max(GetSelStart(), GetSelEnd());
}

void Edit::Draw(const LayoutContext &lc, InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	Window::Draw(lc, ic, dc, texman);

	float w = texman.GetFrameWidth(_font, 0) - 1;

	// selection
	if( GetSelLength() && lc.GetFocused() )
	{
		FRECT rt;
		rt.left = 1 + (GetSelMin() - (float) _offset) * w;
		rt.top = 0;
		rt.right = rt.left + w * GetSelLength() - 1;
		rt.bottom = rt.top + lc.GetSize().y;
		dc.DrawSprite(rt, _selection, 0xffffffff, 0);
	}

	// text
	SpriteColor c = lc.GetEnabled() ? 0xffffffff : 0xaaaaaaaa;
	if( _offset < GetSelMin() )
	{
		dc.DrawBitmapText(0, 1, _font, c, GetText().substr(_offset, GetSelMin() - _offset));
	}
	dc.DrawBitmapText((GetSelMin() - _offset) * w, 1, _font, 0xffff0000, GetText().substr(GetSelMin(), GetSelLength()));
	dc.DrawBitmapText((GetSelMax() - _offset) * w, 1, _font, c, GetText().substr(GetSelMax()));

	float time = GetManager().GetTime() - _lastCursortime;

	// cursor
	if( lc.GetFocused() && fmodf(time, 1.0f) < 0.5f )
	{
		FRECT rt;
		rt.left = (GetSelEnd() - (float) _offset) * w;
		rt.top = 0;
		rt.right = rt.left + texman.GetFrameWidth(_cursor, 0);
		rt.bottom = rt.top + lc.GetSize().y;
		dc.DrawSprite(rt, _cursor, 0xffffffff, 0);
	}
}

bool Edit::OnChar(int c)
{
	if( isprint((unsigned char) c) && '\t' != c )
	{
		int start = GetSelMin();
		SetText(GetManager().GetTextureManager(), GetText().substr(0, start) + (std::string::value_type) c + GetText().substr(GetSelMax()));
		SetSel(start + 1, start + 1);
		return true;
	}
	return false;
}

bool Edit::OnKeyPressed(InputContext &ic, Key key)
{
	TextureManager &texman = GetManager().GetTextureManager();
	bool shift = ic.GetInput().IsKeyPressed(Key::LeftShift) ||
		ic.GetInput().IsKeyPressed(Key::RightShift);
	bool control = ic.GetInput().IsKeyPressed(Key::LeftCtrl) ||
		ic.GetInput().IsKeyPressed(Key::RightCtrl);
	int tmp;
	switch(key)
	{
	case Key::Insert:
		if( shift )
		{
			Paste(texman, ic);
			return true;
		}
		else if( control )
		{
			Copy(ic);
			return true;
		}
		break;
	case Key::V:
		if( control )
		{
			Paste(texman, ic);
			return true;
		}
		break;
	case Key::C:
		if( control )
		{
			Copy(ic);
			return true;
		}
		break;
	case Key::X:
		if( 0 != GetSelLength() && control )
		{
			Copy(ic);
			SetText(texman, GetText().substr(0, GetSelMin()) + GetText().substr(GetSelMax()));
			SetSel(GetSelMin(), GetSelMin());
			return true;
		}
		break;
	case Key::Delete:
		if( 0 == GetSelLength() && GetSelEnd() < GetTextLength() )
		{
			SetText(texman, GetText().substr(0, GetSelStart())
				+ GetText().substr(GetSelEnd() + 1, GetTextLength() - GetSelEnd() - 1));
		}
		else
		{
			if( shift )
			{
				Copy(ic);
			}
			SetText(texman, GetText().substr(0, GetSelMin()) + GetText().substr(GetSelMax()));
		}
		SetSel(GetSelMin(), GetSelMin());
		return true;
	case Key::Backspace:
		tmp = std::max(0, 0 == GetSelLength() ? GetSelStart() - 1 : GetSelMin());
		if( 0 == GetSelLength() && GetSelStart() > 0 )
		{
			SetText(texman, GetText().substr(0, GetSelStart() - 1)
				+ GetText().substr(GetSelEnd(), GetTextLength() - GetSelEnd()));
		}
		else
		{
			SetText(texman, GetText().substr(0, GetSelMin()) + GetText().substr(GetSelMax()));
		}
		SetSel(tmp, tmp);
		return true;
	case Key::Left:
		if( shift )
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
		if( shift )
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
		if( shift )
		{
			SetSel(GetSelStart(), 0);
		}
		else
		{
			SetSel(0, 0);
		}
		return true;
	case Key::End:
		if( shift )
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

bool Edit::OnPointerDown(InputContext &ic, vec2d size, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID)
{
	if( 1 == button && !ic.HasCapturedPointers(this) )
	{
		float w = GetManager().GetTextureManager().GetFrameWidth(_font, 0) - 1;
		int sel = std::min(GetTextLength(), std::max(0, int(pointerPosition.x / w)) + (int) _offset);
		SetSel(sel, sel);
		return true;
	}
	return false;
}

void Edit::OnPointerMove(InputContext &ic, vec2d size, vec2d pointerPosition, PointerType pointerType, unsigned int pointerID, bool captured)
{
	if( captured )
	{
		float w = GetManager().GetTextureManager().GetFrameWidth(_font, 0) - 1;
		int sel = std::min(GetTextLength(), std::max(0, int(pointerPosition.x / w)) + (int) _offset);
		SetSel(GetSelStart(), sel);
	}
}

KeyboardSink* Edit::GetKeyboardSink()
{
	// FIXME: gross hack
	_lastCursortime = GetManager().GetTime();
	return this;
}

void Edit::OnEnabledChange(bool enable, bool inherited)
{
	if( !enable )
	{
		SetSel(0, 0);
		SetFrame(1);
	}
	else
	{
		SetFrame(0);
	}
}

void Edit::OnTextChange(TextureManager &texman)
{
	SetSel(_selStart, _selEnd);
	if( eventChange )
		eventChange();
}

void Edit::Paste(TextureManager &texman, InputContext &ic)
{
	if( const char *data = ic.GetClipboard().GetClipboardText() )
	{
		std::ostringstream buf;
		buf << GetText().substr(0, GetSelMin());
		buf << data;
		buf << GetText().substr(GetSelMax(), GetText().length() - GetSelMax());
		SetText(texman, buf.str());
		SetSel(GetSelMin() + std::strlen(data), GetSelMin() + std::strlen(data));
	}
}

void Edit::Copy(InputContext &ic) const
{
	std::string str = GetText().substr(GetSelMin(), GetSelLength());
	if( !str.empty() )
	{
		ic.GetClipboard().SetClipboardText(std::move(str));
	}
}

