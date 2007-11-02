// Edit.cpp

#include "stdafx.h"

#include "Edit.h"
#include "Text.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

Edit::Edit(Window *parent, float x, float y, float width)
  : Window(parent, x, y, "ctrl_list")
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
	SetSel(_string.length(), _string.length());
	if( eventChange )
		INVOKE(eventChange) ();
}

void Edit::SetInt(int value)
{
	std::ostringstream tmp;
	tmp << value;
	SetText(tmp.str().c_str());
}

int  Edit::GetInt() const
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
	_time = 0;

	_selStart = begin;
	_selEnd   = end;

	float cpos = _selEnd * (_blankText->GetWidth() - 1) - 1;
	if( cpos + _blankText->GetX() > GetWidth() - 10 || cpos + _blankText->GetX() < 10 )
	{
		_blankText->Move(__min(0, GetWidth() * 0.5f - cpos), _blankText->GetY());
	}

	_cursor->Move(cpos + _blankText->GetX(), 0);
	_cursor->Resize(_cursor->GetWidth(), _blankText->GetHeight());

	_selection->Show(_selStart != _selEnd && GetTimeStep());
	_selection->Move(_blankText->GetX() + __min(_selStart, _selEnd) * (_blankText->GetWidth() - 1) + 2, 2);
	_selection->Resize((_blankText->GetWidth() - 1) * abs(_selStart - _selEnd) - 2, _blankText->GetHeight()-2);
}

int Edit::GetSelStart() const
{
	return _selStart;
}

int Edit::GetSelEnd() const
{
	return _selEnd;
}

void Edit::OnChar(int c)
{
	if( isprint((unsigned char) c) )
	{
		int start = __min(_selStart, _selEnd);
		_string = _string.substr(0, start) + (char) c + _string.substr(__max(_selStart, _selEnd));
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
		if( _selStart != _selEnd && (GetAsyncKeyState(VK_CONTROL) & 0x8000) )
		{
			Copy();
			_string = _string.substr(0, __min(_selStart, _selEnd))
				+ _string.substr(__max(_selStart, _selEnd));
			_blankText->SetText(_string.c_str());
			tmp = __min(_selStart, _selEnd);
			SetSel(tmp, tmp);
			if( eventChange )
				INVOKE(eventChange) ();
		}
		break;
	case VK_DELETE:
		if( _selStart == _selEnd && _selEnd < (int) _string.length() )
		{
			_string = _string.substr(0, _selStart)
				+ _string.substr(_selEnd + 1, _string.length() - _selEnd - 1);
		}
		else
		{
			if( GetAsyncKeyState(VK_SHIFT) & 0x8000 )
			{
				Copy();
			}
			_string = _string.substr(0, __min(_selStart, _selEnd))
				+ _string.substr(__max(_selStart, _selEnd));
		}
		_blankText->SetText(_string.c_str());
		tmp = __min(_selStart, _selEnd);
		SetSel(tmp, tmp);
		if( eventChange )
			INVOKE(eventChange) ();
		break;
	case VK_BACK:
		if( _selStart == _selEnd && _selStart > 0 )
		{
			_string = _string.substr(0, _selStart - 1)
				+ _string.substr(_selEnd, _string.length() - _selEnd);
		}
		else
		{
			_string = _string.substr(0, __min(_selStart, _selEnd))
				+ _string.substr(__max(_selStart, _selEnd));
		}
		tmp = __max(0, _selEnd == _selStart ? _selStart - 1 : __min(_selStart, _selEnd));
		SetSel(tmp, tmp);
		_blankText->SetText(_string.c_str());
		if( eventChange )
			INVOKE(eventChange) ();
		break;
	case VK_LEFT:
		tmp = __max(0, __min(_selStart, _selEnd) - 1);
		if( GetAsyncKeyState(VK_SHIFT) & 0x8000 )
		{
			SetSel(_selStart, tmp);
		}
		else
		{
			SetSel(tmp, tmp);
		}
		break;
	case VK_RIGHT:
		tmp = __min((int) _string.length(), __max(_selStart, _selEnd) + 1);
		if( GetAsyncKeyState(VK_SHIFT) & 0x8000 )
		{
			SetSel(_selStart, tmp);
		}
		else
		{
			SetSel(tmp, tmp);
		}
		break;
	case VK_HOME:
		if( GetAsyncKeyState(VK_SHIFT) & 0x8000 )
		{
			SetSel(_selStart, 0);
		}
		else
		{
			SetSel(0, 0);
		}
		break;
	case VK_END:
		if( GetAsyncKeyState(VK_SHIFT) & 0x8000 )
		{
			SetSel(_selStart, _string.length());
		}
		else
		{
			SetSel(_string.length(), _string.length());
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
		int sel = __min(_string.length(), (size_t) __max(0,
			(x - _blankText->GetX() + _blankText->GetWidth() / 2) / (_blankText->GetWidth() - 1)));
		SetSel(sel, sel);
	}
	return true;
}

bool Edit::OnMouseMove(float x, float y)
{
	if( IsCaptured() )
	{
		int sel = __min(_string.length(), (size_t) __max(0,
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
	_selection->Show(_selStart != _selEnd && focus);
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
				_string = _string.substr(0, _selStart)
					+ data
					+ _string.substr(_selEnd, _string.length() - _selEnd - 1);
				_blankText->SetText(_string.c_str());
				SetSel(_selStart + strlen(data), _selStart + strlen(data));
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
	string_t str = GetText().substr(__min(_selStart, _selEnd), abs(GetSelEnd() - GetSelStart()));
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

