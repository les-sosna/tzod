// Edit.cpp

#include "stdafx.h"

#include "Edit.h"

#include "video/TextureManager.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

Edit::Edit(Window *parent, float x, float y, float width)
  : Window(parent, x, y, "ui/list")
  , _selStart(-1)
  , _selEnd(-1)
  , _selection(NULL)
  , _cursor(NULL)
  , _offset(0)
{
	SetBorder(true);
	SetClipChildren(true);

	_font = g_texman->FindSprite("font_small");

	_cursor    = new Window(this, 0, 0, "ui/editcursor");
	_cursor->SetVisible(false);

	_selection = new Window(this, 0, 0, "ui/editsel");
	_selection->SetBorder(true);

	Move(x, y);
	Resize(width, g_texman->Get(_font).pxFrameHeight + 2);

	SetSel(0, 0);

	_time = 0;
}

const string_t& Edit::GetText() const
{
	return _string;
}

void Edit::SetText(const string_t &text)
{
	_string.swap(string_t());
	_string = text;
	SetSel(-1, -1);
	if( eventChange )
		INVOKE(eventChange) ();
}

int Edit::GetTextLength() const
{
	return (int) _string.length();
}

void Edit::SetInt(int value)
{
	ostrstream_t tmp;
	tmp << value;
	SetText(tmp.str());
}

int Edit::GetInt() const
{
	istrstream_t tmp(_string);
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
	istrstream_t tmp(_string);
	float result = 0;
	tmp >> result;
	return result;
}

void Edit::SetSel(int begin, int end)
{
	assert(begin >= -1 && begin <= GetTextLength());
	assert(end >= -1 && end <= GetTextLength());

	_time = 0;

	_selStart = (-1 != begin) ? begin : GetTextLength();
	_selEnd   = (-1 != end)   ? end   : GetTextLength();

	float w = g_texman->Get(_font).pxFrameWidth - 1;
	float h = g_texman->Get(_font).pxFrameHeight;

	float cpos = _selEnd * w - 1;
	if( cpos - (float) (_offset * w) > GetWidth() - 10 || cpos - (float) (_offset * w) < 10 )
	{
		_offset = size_t(std::max<float>(0, cpos - GetWidth() * 0.5f) / w);
	}

	_cursor->Move(cpos - (float) (_offset * w), 0);
	_cursor->Resize(_cursor->GetWidth(), h);

	_selection->SetVisible(_selStart != _selEnd && GetTimeStep());
	_selection->Move(GetSelMin() * w + 2 - (float) (_offset * w), 2);
	_selection->Resize(w * GetSelLength() - 2, h - 2);
}

int Edit::GetSelStart() const
{
	return _selStart;
}

int Edit::GetSelEnd() const
{
	return _selEnd;
}

int Edit::GetSelLength() const
{
	return abs(_selStart - _selEnd);
}

int Edit::GetSelMin() const
{
	return __min(_selStart, _selEnd);
}

int Edit::GetSelMax() const
{
	return __max(_selStart, _selEnd);
}

void Edit::DrawChildren(float sx, float sy) const
{
	SpriteColor c = GetEnabled() ? 0xffffffff : 0xaaaaaaaa;
	float w = g_texman->Get(_font).pxFrameWidth - 1;
	g_texman->DrawBitmapText(_font, _string.substr(_offset), c, sx, sy + 1);

	Window::DrawChildren(sx, sy);
}

void Edit::OnChar(int c)
{
	if( isprint((unsigned char) c) && VK_TAB != c )
	{
		int start = GetSelMin();
		_string = _string.substr(0, start) + (string_t::value_type) c + _string.substr(GetSelMax());
		SetSel(start + 1, start + 1);
		if( eventChange )
			INVOKE(eventChange) ();
	}
}

void Edit::OnRawChar(int c)
{
	int tmp;

	switch(c)
	{
	case VK_INSERT:
		if( GetAsyncKeyState(VK_SHIFT) & 0x8000 )
		{
			Paste();
		}
		else if( GetAsyncKeyState(VK_CONTROL) & 0x8000 )
		{
			Copy();
		}
		break;
	case 'V':
		if( GetAsyncKeyState(VK_CONTROL) & 0x8000 )
		{
			Paste();
		}
		break;
	case 'C':
		if( GetAsyncKeyState(VK_CONTROL) & 0x8000 )
		{
			Copy();
		}
		break;
	case 'X':
		if( 0 != GetSelLength() && (GetAsyncKeyState(VK_CONTROL) & 0x8000) )
		{
			Copy();
			_string = _string.substr(0, GetSelMin()) + _string.substr(GetSelMax());
			SetSel(GetSelMin(), GetSelMin());
			if( eventChange )
				INVOKE(eventChange) ();
		}
		break;
	case VK_DELETE:
		if( 0 == GetSelLength() && GetSelEnd() < GetTextLength() )
		{
			_string = _string.substr(0, GetSelStart())
				+ _string.substr(GetSelEnd() + 1, GetTextLength() - GetSelEnd() - 1);
		}
		else
		{
			if( GetAsyncKeyState(VK_SHIFT) & 0x8000 )
			{
				Copy();
			}
			_string = _string.substr(0, GetSelMin()) + _string.substr(GetSelMax());
		}
		SetSel(GetSelMin(), GetSelMin());
		if( eventChange )
			INVOKE(eventChange) ();
		break;
	case VK_BACK:
		if( 0 == GetSelLength() && GetSelStart() > 0 )
		{
			_string = _string.substr(0, GetSelStart() - 1)
				+ _string.substr(GetSelEnd(), GetTextLength() - GetSelEnd());
		}
		else
		{
			_string = _string.substr(0, GetSelMin()) + _string.substr(GetSelMax());
		}
		tmp = __max(0, 0 == GetSelLength() ? GetSelStart() - 1 : GetSelMin());
		SetSel(tmp, tmp);
		if( eventChange )
			INVOKE(eventChange) ();
		break;
	case VK_LEFT:
		if( GetAsyncKeyState(VK_SHIFT) & 0x8000 )
		{
			tmp = __max(0, GetSelEnd() - 1);
			SetSel(GetSelStart(), tmp);
		}
		else
		{
			tmp = __max(0, GetSelMin() - 1);
			SetSel(tmp, tmp);
		}
		break;
	case VK_RIGHT:
		if( GetAsyncKeyState(VK_SHIFT) & 0x8000 )
		{
			tmp = __min(GetTextLength(), GetSelEnd() + 1);
			SetSel(GetSelStart(), tmp);
		}
		else
		{
			tmp = __min(GetTextLength(), GetSelMax() + 1);
			SetSel(tmp, tmp);
		}
		break;
	case VK_HOME:
		if( GetAsyncKeyState(VK_SHIFT) & 0x8000 )
		{
			SetSel(GetSelStart(), 0);
		}
		else
		{
			SetSel(0, 0);
		}
		break;
	case VK_END:
		if( GetAsyncKeyState(VK_SHIFT) & 0x8000 )
		{
			SetSel(GetSelStart(), -1);
		}
		else
		{
			SetSel(-1, -1);
		}
		break;
	default:
		GetParent()->OnRawChar(c);
	}
}

bool Edit::OnMouseDown(float x, float y, int button)
{
	if( 1 == button )
	{
		SetCapture();
		float w = g_texman->Get(_font).pxFrameWidth - 1;
		int sel = __min(GetTextLength(), (int) __max(0, x / w) + (int) _offset);
		SetSel(sel, sel);
		return true;
	}
	return false;
}

bool Edit::OnMouseMove(float x, float y)
{
	if( IsCaptured() )
	{
		float w = g_texman->Get(_font).pxFrameWidth - 1;
		int sel = __min(GetTextLength(), (int) __max(0, x / w) + (int) _offset);
		SetSel(GetSelStart(), sel);
		return true;
	}
	return false;
}

bool Edit::OnMouseUp(float x, float y, int button)
{
	if( 1 == button && IsCaptured() )
	{
		ReleaseCapture();
		return true;
	}
	return false;
}

bool Edit::OnFocus(bool focus)
{
	SetTimeStep(focus);
	_cursor->SetVisible(focus);
	_selection->SetVisible(0 != GetSelLength() && focus);
	_time = 0;
	return true;
}

void Edit::OnEnabledChange(bool enable, bool inherited)
{
	if( !enable )
	{
		SetSel(0, 0);
	}
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
		if( HANDLE hData = GetClipboardData(CF_OEMTEXT) )
		{
			if( const char *data = (const char *) GlobalLock(hData) )
			{
				ostrstream_t buf;
				buf << _string.substr(0, GetSelMin());
				buf << data;
				buf << _string.substr(GetSelMax(), _string.length() - GetSelMax());
				_string.swap(buf.str());
				SetSel(GetSelMin() + strlen(data), GetSelMin() + strlen(data));
				GlobalUnlock(hData);
				if( eventChange )
					INVOKE(eventChange) ();
			}
			else
			{
//				TRACE(_T("Failed to lock data: %d\n"), GetLastError());
			}
		}
		else
		{
//			TRACE(_T("Failed to get clipboard data: %d\n"), GetLastError());
		}
		CloseClipboard();
	}
	else
	{
//		TRACE(_T("Failed to open clipboard: %d\n"), GetLastError());
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
//					TRACE(_T("Failed to allocate memory: %d\n"), GetLastError());
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
//					TRACE(_T("Failed to set clipboard data: %d\n"), GetLastError());
				}
			}
			else
			{
//				TRACE(_T("Failed to empty clipboard: %d\n"), GetLastError());
			}
			CloseClipboard();
		}
		else
		{
//			TRACE(_T("Failed to open clipboard: %d\n"), GetLastError());
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

