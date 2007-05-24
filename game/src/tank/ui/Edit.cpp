// Edit.cpp

#include "stdafx.h"

#include "Edit.h"
#include "Text.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

Edit::Edit(Window *parent, float x, float y, float width)
  : Window(parent)
{
	SetBorder(true);

	_blankText = new Text(this, 1, 1, "", alignTextLT);
	_cursor    = new Window(this, 0, 0, "ctrl_editcursor");
	_cursor->Show(false);

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

void Edit::SetSel(int begin, int end)
{
	_time = 0;

	_selStart = begin;
	_selEnd   = end;

	_cursor->Move(_selEnd * (_blankText->GetWidth() - 1), 0);
	_cursor->Resize(_cursor->GetWidth(), _blankText->GetHeight());
}

void Edit::OnChar(int c)
{
	switch(c)
	{
	case VK_BACK:  // skip non-printable keys
	case VK_TAB:
	case VK_ESCAPE:
	case VK_RETURN:
		break;
	default:
		_string = _string.substr(0, _selStart) + (char) c
			+ _string.substr(_selEnd, _string.length() - _selEnd);
		SetSel(_selStart+1, _selStart+1);
		_blankText->SetText(_string.c_str());
	}
}

void Edit::OnRawChar(int c)
{
	int tmp;

	switch(c)
	{
	case VK_DELETE:
		if( _selStart == _selEnd && _selEnd < (int) _string.length() )
		{
			_string = _string.substr(0, _selStart)
				+ _string.substr(_selEnd + 1, _string.length() - _selEnd - 1);
		}
		else
		{
			_string = _string.substr(0, _selStart)
				+ _string.substr(_selEnd, _string.length() - _selEnd);
		}
		_blankText->SetText(_string.c_str());
		break;
	case VK_BACK:
		if( _selStart == _selEnd && _selStart > 0 )
		{
			_string = _string.substr(0, _selStart - 1)
				+ _string.substr(_selEnd, _string.length() - _selEnd);
		}
		else
		{
			_string = _string.substr(0, _selStart)
				+ _string.substr(_selEnd, _string.length() - _selEnd);
		}
		tmp = __max(0, __min(_selStart, _selEnd) - 1);
		SetSel(tmp, tmp);
		_blankText->SetText(_string.c_str());
		break;
	case VK_LEFT:
		tmp = __max(0, __min(_selStart, _selEnd) - 1);
		SetSel(tmp, tmp);
		break;
	case VK_RIGHT:
		tmp = __min((int) _string.length(), __min(_selStart, _selEnd) + 1);
		SetSel(tmp, tmp);
		break;
	case VK_HOME:
		SetSel(0, 0);
		break;
	case VK_END:
		SetSel(_string.length(), _string.length());
		break;
	default:
		GetParent()->OnRawChar(c);
	}
}

bool Edit::OnMouseDown(float x, float y, int button)
{

	return true;
}

void Edit::Draw(float sx, float sy)
{
	Window::Draw(sx, sy);

//	_blankText->SetText(_string);
//	_blankText->Show
    
}

bool Edit::OnFocus(bool focus)
{
	SetTimeStep(focus);
	_cursor->Show(focus);
	_time = 0;
	return true;
}

void Edit::OnTimeStep(float dt)
{
	_time += dt;
	_cursor->Show(fmodf(_time, 1.0f) < 0.5f);
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

