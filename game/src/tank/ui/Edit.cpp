// Edit.cpp

#include "stdafx.h"

#include "Edit.h"
#include "GuiManager.h"

#include "video/TextureManager.h"

namespace UI
{

Edit* Edit::Create(Window *parent, float x, float y, float width)
{
	Edit *res = new Edit(parent);
	res->Move(x, y);
	res->Resize(width, res->GetHeight());
	return res;
}

///////////////////////////////////////////////////////////////////////////////

Edit::Edit(Window *parent)
  : Window(parent)
  , _selStart(-1)
  , _selEnd(-1)
  , _selection(NULL)
  , _cursor(NULL)
  , _offset(0)
  , _time(0)
{
	_font = GetManager()->GetTextureManager()->FindSprite("font_small");

	_cursor = Window::Create(this);
	_cursor->SetTexture("ui/editcursor", true);
	_cursor->SetVisible(false);

	_selection = Window::Create(this);
	_selection->SetTexture("ui/editsel", true);
	_selection->SetDrawBorder(true);

	SetTexture("ui/edit", true);
	SetDrawBorder(true);
	SetClipChildren(true);
	SetSel(0, 0);
	Resize(GetWidth(), GetManager()->GetTextureManager()->GetCharHeight(_font));
}

int Edit::GetTextLength() const
{
	return (int) GetText().length();
}

void Edit::SetInt(int value)
{
	ostrstream_t tmp;
	tmp << value;
	SetText(tmp.str());
}

int Edit::GetInt() const
{
	istrstream_t tmp(GetText());
	int result = 0;
	tmp >> result;
	return result;
}

void Edit::SetFloat(float value)
{
	ostrstream_t tmp;
	tmp << value;
	SetText(tmp.str());
}

float Edit::GetFloat() const
{
	istrstream_t tmp(GetText());
	float result = 0;
	tmp >> result;
	return result;
}

void Edit::SetSel(int begin, int end)
{
	assert(begin >= -1 && end >= -1);

	_selStart = begin <= GetTextLength() ? begin : -1;
	_selEnd   = end <= GetTextLength() ? end : -1;
	_time     = 0;

	float w = GetManager()->GetTextureManager()->GetFrameWidth(_font, 0) - 1;

	float cpos = GetSelEnd() * w;
	if( cpos - (float) (_offset * w) > GetWidth() - 10 || cpos - (float) (_offset * w) < 10 )
	{
		_offset = size_t(std::max<float>(0, cpos - GetWidth() * 0.5f) / w);
	}

	_cursor->Move(cpos - (float) (_offset * w), 0);
	_cursor->Resize(_cursor->GetWidth(), GetHeight());

	_selection->SetVisible(GetSelLength() && GetTimeStep());
	_selection->Move(GetSelMin() * w + 1 - (float) (_offset * w), 0);
	_selection->Resize(w * GetSelLength(), GetHeight());
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

void Edit::DrawChildren(const DrawingContext *dc, float sx, float sy) const
{
	Window::DrawChildren(dc, sx, sy);

	SpriteColor c = GetEnabled() ? 0xffffffff : 0xaaaaaaaa;
	float w = dc->GetFrameWidth(_font, 0) - 1;

	if( _offset < GetSelMin() )
	{
		dc->DrawBitmapText(sx, sy, _font, c, GetText().substr(_offset, GetSelMin() - _offset));
	}

	dc->DrawBitmapText(sx + (GetSelMin() - _offset) * w, sy, _font, 0xffff0000, GetText().substr(GetSelMin(), GetSelLength()));
	dc->DrawBitmapText(sx + (GetSelMax() - _offset) * w, sy, _font, c, GetText().substr(GetSelMax()));
}

bool Edit::OnChar(int c)
{
	if( isprint((unsigned char) c) && VK_TAB != c )
	{
		int start = GetSelMin();
		SetText(GetText().substr(0, start) + (string_t::value_type) c + GetText().substr(GetSelMax()));
		SetSel(start + 1, start + 1);
		return true;
	}
	return false;
}

bool Edit::OnRawChar(int c)
{
	int tmp;

	switch(c)
	{
	case VK_INSERT:
		if( GetAsyncKeyState(VK_SHIFT) & 0x8000 )
		{
			Paste();
			return true;
		}
		else if( GetAsyncKeyState(VK_CONTROL) & 0x8000 )
		{
			Copy();
			return true;
		}
		break;
	case 'V':
		if( GetAsyncKeyState(VK_CONTROL) & 0x8000 )
		{
			Paste();
			return true;
		}
		break;
	case 'C':
		if( GetAsyncKeyState(VK_CONTROL) & 0x8000 )
		{
			Copy();
			return true;
		}
		break;
	case 'X':
		if( 0 != GetSelLength() && (GetAsyncKeyState(VK_CONTROL) & 0x8000) )
		{
			Copy();
			SetText(GetText().substr(0, GetSelMin()) + GetText().substr(GetSelMax()));
			SetSel(GetSelMin(), GetSelMin());
			return true;
		}
		break;
	case VK_DELETE:
		if( 0 == GetSelLength() && GetSelEnd() < GetTextLength() )
		{
			SetText(GetText().substr(0, GetSelStart())
				+ GetText().substr(GetSelEnd() + 1, GetTextLength() - GetSelEnd() - 1));
		}
		else
		{
			if( GetAsyncKeyState(VK_SHIFT) & 0x8000 )
			{
				Copy();
			}
			SetText(GetText().substr(0, GetSelMin()) + GetText().substr(GetSelMax()));
		}
		SetSel(GetSelMin(), GetSelMin());
		return true;
	case VK_BACK:
		tmp = std::max(0, 0 == GetSelLength() ? GetSelStart() - 1 : GetSelMin());
		if( 0 == GetSelLength() && GetSelStart() > 0 )
		{
			SetText(GetText().substr(0, GetSelStart() - 1)
				+ GetText().substr(GetSelEnd(), GetTextLength() - GetSelEnd()));
		}
		else
		{
			SetText(GetText().substr(0, GetSelMin()) + GetText().substr(GetSelMax()));
		}
		SetSel(tmp, tmp);
		return true;
	case VK_LEFT:
		if( GetAsyncKeyState(VK_SHIFT) & 0x8000 )
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
	case VK_RIGHT:
		if( GetAsyncKeyState(VK_SHIFT) & 0x8000 )
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
	case VK_HOME:
		if( GetAsyncKeyState(VK_SHIFT) & 0x8000 )
		{
			SetSel(GetSelStart(), 0);
		}
		else
		{
			SetSel(0, 0);
		}
		return true;
	case VK_END:
		if( GetAsyncKeyState(VK_SHIFT) & 0x8000 )
		{
			SetSel(GetSelStart(), -1);
		}
		else
		{
			SetSel(-1, -1);
		}
		return true;
	case VK_SPACE:
		return true;
	}
	return false;
}

bool Edit::OnMouseDown(float x, float y, int button)
{
	if( 1 == button )
	{
		GetManager()->SetCapture(this);
		float w = GetManager()->GetTextureManager()->GetFrameWidth(_font, 0) - 1;
		int sel = std::min(GetTextLength(), std::max(0, int(x / w)) + (int) _offset);
		SetSel(sel, sel);
		return true;
	}
	return false;
}

bool Edit::OnMouseMove(float x, float y)
{
	if( GetManager()->GetCapture() == this )
	{
		float w = GetManager()->GetTextureManager()->GetFrameWidth(_font, 0) - 1;
		int sel = std::min(GetTextLength(), std::max(0, int(x / w)) + (int) _offset);
		SetSel(GetSelStart(), sel);
		return true;
	}
	return false;
}

bool Edit::OnMouseUp(float x, float y, int button)
{
	if( 1 == button && GetManager()->GetCapture() == this )
	{
		GetManager()->SetCapture(NULL);
		return true;
	}
	return false;
}

bool Edit::OnFocus(bool focus)
{
	SetTimeStep(focus);
	_cursor->SetVisible(focus);
	_selection->SetBackColor(focus ? 0xffffffff : 0x00000000);
	_time = 0;
	return true;
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

void Edit::OnTextChange()
{
	SetSel(_selStart, _selEnd);
	if( eventChange )
		INVOKE(eventChange) ();
}

void Edit::OnTimeStep(float dt)
{
	_time += dt;
	_cursor->SetVisible(fmodf(_time, 1.0f) < 0.5f);
}

void Edit::Paste()
{
	if( OpenClipboard(NULL) )
	{
#ifdef _UNICODE
		if( HANDLE hData = GetClipboardData(CF_UNICODETEXT) )
#else
		if( HANDLE hData = GetClipboardData(CF_OEMTEXT) )
#endif
		{
			if( const char *data = (const char *) GlobalLock(hData) )
			{
				string_t data1(data);
				GlobalUnlock(hData);

				ostrstream_t buf;
				buf << GetText().substr(0, GetSelMin());
				buf << data1;
				buf << GetText().substr(GetSelMax(), GetText().length() - GetSelMax());
				SetText(buf.str());
				SetSel(GetSelMin() + data1.length(), GetSelMin() + data1.length());
			}
			else
			{
//				TRACE(_T("Failed to lock data: %d"), GetLastError());
			}
		}
		else
		{
//			TRACE(_T("Failed to get clipboard data: %d"), GetLastError());
		}
		CloseClipboard();
	}
	else
	{
//		TRACE(_T("Failed to open clipboard: %d"), GetLastError());
	}
}

void Edit::Copy() const
{
	string_t str = GetText().substr(GetSelMin(), GetSelLength());
	if( !str.empty() )
	{
		if( OpenClipboard(NULL) )
		{
			if( EmptyClipboard() )
			{
				// Allocate a global memory object for the text.
				HANDLE hData = GlobalAlloc(GMEM_MOVEABLE, (str.length() + 1) * sizeof(string_t::value_type));
				if( NULL == hData )
				{
//					TRACE(_T("Failed to allocate memory: %d"), GetLastError());
					CloseClipboard();
					return;
				}

				// Lock the handle and copy the text to the buffer.
				char *data = (char *) GlobalLock(hData);
				memcpy(data, str.c_str(), (str.length() + 1) * sizeof(string_t::value_type));
				GlobalUnlock(hData);

				// Place the handle on the clipboard.
#ifdef UNICODE
				if( !SetClipboardData(CF_UNICODETEXT, hData) )
#else
				if( !SetClipboardData(CF_OEMTEXT, hData) )
#endif
				{
//					TRACE(_T("Failed to set clipboard data: %d"), GetLastError());
				}
			}
			else
			{
//				TRACE(_T("Failed to empty clipboard: %d"), GetLastError());
			}
			CloseClipboard();
		}
		else
		{
//			TRACE(_T("Failed to open clipboard: %d"), GetLastError());
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

