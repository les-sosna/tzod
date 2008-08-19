// Edit.cpp

#include "stdafx.h"

#include "Edit.h"
#include "Text.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

Edit::Edit(Window *parent, float x, float y, float width)
  : Window(parent, x, y, "ctrl_list")
  , _selStart(-1)
  , _selEnd(-1)
  , _selection(NULL)
  , _blankText(NULL)
  , _cursor(NULL)
{
	SetBorder(true);
	ClipChildren(true);

	_blankText = new Text(this, 1, 1, "", alignTextLT);
	_cursor    = new Window(this, 0, 0, "ctrl_editcursor");
	_cursor->Show(false);

	_selection = new Window(this, 0, 0, "ctrl_editsel");
	_selection->SetBorder(true);

	Move(x, y);
	Resize(width, _blankText->GetHeight() + 2);

	SetSel(0, 0);

	_time = 0;
}

const string_t& Edit::GetText() const
{
	return _string;
}

void Edit::SetText(const char *text)
{
	_string = text;
	_blankText->SetText(text);
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
	std::ostringstream tmp;
	tmp << value;
	SetText(tmp.str().c_str());
}

int Edit::GetInt() const
{
	return atoi(_string.c_str());
}

void Edit::SetFloat(float value)
{
	std::ostringstream tmp;
	tmp << value;
	SetText(tmp.str().c_str());
}

float Edit::GetFloat() const
{
	return (float) atof(_string.c_str());
}

void Edit::SetSel(int begin, int end)
{
	_ASSERT(begin >= -1 && begin <= GetTextLength());
	_ASSERT(end >= -1 && end <= GetTextLength());

	_time = 0;

	_selStart = (-1 != begin) ? begin : GetTextLength();
	_selEnd   = (-1 != end)   ? end   : GetTextLength();

	float cpos = _selEnd * (_blankText->GetWidth() - 1) - 1;
	if( cpos + _blankText->GetX() > GetWidth() - 10 || cpos + _blankText->GetX() < 10 )
	{
		_blankText->Move(__min(0, GetWidth() * 0.5f - cpos), _blankText->GetY());
	}

	_cursor->Move(cpos + _blankText->GetX(), 0);
	_cursor->Resize(_cursor->GetWidth(), _blankText->GetHeight());

	_selection->Show(_selStart != _selEnd && GetTimeStep());
	_selection->Move(_blankText->GetX() + GetSelMin() * (_blankText->GetWidth() - 1) + 2, 2);
	_selection->Resize((_blankText->GetWidth() - 1) * GetSelLength() - 2, _blankText->GetHeight() - 2);
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

void Edit::OnChar(int c)
{
	if( isprint((unsigned char) c) && VK_TAB != c )
	{
		int start = GetSelMin();
		_string = _string.substr(0, start) + (char) c + _string.substr(GetSelMax());
		SetSel(start + 1, start + 1);
		_blankText->SetText(_string.c_str());
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
			_blankText->SetText(_string.c_str());
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
		_blankText->SetText(_string.c_str());
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
		_blankText->SetText(_string.c_str());
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
		int sel = __min(GetTextLength(), (int) __max(0,
			(x - _blankText->GetX() + _blankText->GetWidth() / 2) / (_blankText->GetWidth() - 1)));
		SetSel(sel, sel);
	}
	return true;
}

bool Edit::OnMouseMove(float x, float y)
{
	if( IsCaptured() )
	{
		int sel = __min(GetTextLength(), (int) __max(0,
			(x - _blankText->GetX() + _blankText->GetWidth() / 2) / (_blankText->GetWidth() - 1)));
		SetSel(GetSelStart(), sel);
	}
	return true;
}

bool Edit::OnMouseUp(float x, float y, int button)
{
	if( IsCaptured() )
	{
		ReleaseCapture();
	}
	return true;
}

bool Edit::OnFocus(bool focus)
{
	SetTimeStep(focus);
	_cursor->Show(focus);
	_selection->Show(0 != GetSelLength() && focus);
	_time = 0;
	return true;
}

void Edit::OnEnable(bool enable)
{
	if( enable )
	{
		_blankText->SetColor(0xffffffff);
	}
	else
	{
		SetSel(0, 0);
		_blankText->SetColor(0xaaaaaaaa);
	}
}

void Edit::OnTimeStep(float dt)
{
	_time += dt;
	_cursor->Show(fmodf(_time, 1.0f) < 0.5f);
}

void Edit::Paste()
{
	if( OpenClipboard(NULL) )
	{
		if( HANDLE hData = GetClipboardData(CF_OEMTEXT) )
		{
			if( const char *data = (const char *) GlobalLock(hData) )
			{
				_string = _string.substr(0, GetSelMin())
					+ data
					+ _string.substr(GetSelMax(), _string.length() - GetSelMax());
				_blankText->SetText(_string.c_str());
				SetSel(GetSelMin() + strlen(data), GetSelMin() + strlen(data));
				GlobalUnlock(hData);
				if( eventChange )
					INVOKE(eventChange) ();
			}
			else
			{
			//	TRACE(_T("Failed to lock data: %d\n"), GetLastError());
			}
		}
		else
		{
		//	TRACE(_T("Failed to get clipboard data: %d\n"), GetLastError());
		}
		CloseClipboard();
	}
	else
	{
	//	TRACE(_T("Failed to open clipboard: %d\n"), GetLastError());
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
				HANDLE hData = GlobalAlloc(GMEM_MOVEABLE, (str.length() + 1) * sizeof(char));
				if( NULL == hData )
				{
		//			TRACE(_T("Failed to allocate memory: %d\n"), GetLastError());
					CloseClipboard();
					return;
				}

				// Lock the handle and copy the text to the buffer.
				char *data = (char *) GlobalLock(hData);
				memcpy(data, str.c_str(), str.length());
				data[str.length()] = '\0';    // null character
				GlobalUnlock(hData);

				// Place the handle on the clipboard.
				if( !SetClipboardData(CF_OEMTEXT, hData) )
				{
				//	TRACE(_T("Failed to set clipboard data: %d\n"), GetLastError());
				}
			}
			else
			{
			//	TRACE(_T("Failed to empty clipboard: %d\n"), GetLastError());
			}
			CloseClipboard();
		}
		else
		{
		//	TRACE(_T("Failed to open clipboard: %d\n"), GetLastError());
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

