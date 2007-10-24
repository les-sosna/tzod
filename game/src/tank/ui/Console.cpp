// Console.cpp

#include "stdafx.h"

#include "console.h"
#include "Text.h"
#include "Edit.h"

#include "core/Console.h"
#include "config/Config.h"

#include "GuiManager.h"


namespace UI
{
///////////////////////////////////////////////////////////////////////////////


Console::Console(Window *parent, float x, float y)
  : Window(parent, x, y, "window")
{
	SetBorder(true);

	_arrow = new Text(this, 0, 0, ">", alignTextLT);

	_blankText = new Text(this, 0, 0, "", alignTextLT);
	_blankText->Show(false);

	_input = new Edit(this, 0, 0, 0);
	_input->SetTexture(NULL);

	_scrollBack = 0;
	_cmdIndex   = g_conf.con_history->GetSize();
}

void Console::OnChar(int c)
{
	GetManager()->SetFocusWnd(_input);
}

void Console::OnRawChar(int c)
{
	switch(c)
	{
	case VK_UP:
		if( _cmdIndex > g_conf.con_history->GetSize() )
		{
			_cmdIndex = g_conf.con_history->GetSize();
		}
		if( _cmdIndex > 0 )
		{
			_input->SetText(g_conf.con_history->GetStr(--_cmdIndex, "")->Get());
		}
		break;
	case VK_DOWN:
		++_cmdIndex;
		if( _cmdIndex < g_conf.con_history->GetSize() )
		{
			_input->SetText(g_conf.con_history->GetStr(_cmdIndex, "")->Get());
		}
		else
		{
			_input->SetText("");
			_cmdIndex = g_conf.con_history->GetSize();
		}
		break;
	case VK_RETURN:
	{
		const string_t &cmd = _input->GetText();
		if( cmd.empty() )
		{
			g_console->printf(">\n");
		}
		else
		{
			_scrollBack = 0;

			if( g_conf.con_history->GetSize() == 0 ||
				cmd != g_conf.con_history->GetStr(g_conf.con_history->GetSize()-1, "")->Get() )
			{
				g_conf.con_history->PushBack(ConfVar::typeString)->AsStr()->Set(cmd.c_str());
				while( g_conf.con_history->GetSize() > (unsigned) g_conf.con_maxhistory->GetInt() )
					g_conf.con_history->PopFront();
			}
			_cmdIndex = g_conf.con_history->GetSize();

			g_console->printf("> %s\n", cmd.c_str());      // echo to console
			if( eventOnSendCommand )
				INVOKE(eventOnSendCommand) (cmd.c_str());  // send to the scripting system
			_input->SetText("");                           // erase input field
		}
		break;
	}
	case VK_PRIOR:
		_scrollBack = __min(_scrollBack + 1, g_console->GetLineCount() - 1);
		break;
	case VK_NEXT:
		if( _scrollBack > 0 ) --_scrollBack;
		break;
	case VK_HOME:
		_scrollBack = g_console->GetLineCount() - 1;
		break;
	case VK_END:
		_scrollBack = 0;
		break;
	case VK_OEM_3:
		Show(false);
		GetManager()->SetFocusWnd(NULL);
		break;
	case VK_ESCAPE:
		if( !_input->GetText().empty() )
		{
			_input->SetText("");
		}
		else
		{
			Show(false);
			GetManager()->SetFocusWnd(NULL);
		}
		break;
	case VK_TAB:
		if( eventOnRequestCompleteCommand )
		{
			string_t result;
			HRESULT hr = INVOKE(eventOnRequestCompleteCommand) 
				(_input->GetText().substr(0, _input->GetSelEnd()).c_str(), result);
			if( SUCCEEDED(hr) )
			{
				int end = _input->GetSelEnd();
				_input->SetText( 
					( _input->GetText().substr(0, end)
					+ result
					+ _input->GetText().substr(end)
					).c_str()
					);
				_input->SetSel(end + result.length(), end + result.length());
			}
		}
		break;
	}
}

bool Console::OnMouseWheel(float x, float y, float z)
{
	if( z > 0 )
	{
		_scrollBack = __min(_scrollBack + int(z * 3), g_console->GetLineCount() - 1);
	}
	else
	{
		size_t dz = __min((signed) _scrollBack, int(-z * 3));
		_scrollBack -= dz;
	}
	return true;
}

bool Console::OnMouseDown(float x, float y, int button)
{
	return true;
}

void Console::DrawChildren(float sx, float sy)
{
	Window::DrawChildren(sx, sy);
	_blankText->Show(true);
	size_t visibleLineCount = (size_t) (GetHeight() / _blankText->GetHeight());
	size_t scroll = __min(_scrollBack, g_console->GetLineCount());
	size_t count  = __min( g_console->GetLineCount() - scroll, visibleLineCount );
	for( size_t i = 0; i < count; ++i )
	{
		_blankText->SetText( g_console->GetLine(g_console->GetLineCount() - scroll - i - 1) );
		_blankText->Draw(sx + 4, sy -= _blankText->GetHeight());
	}
	_blankText->Show(false);
}

void Console::OnShow(bool show)
{
	if( show )
		GetManager()->SetFocusWnd(this);
}

void Console::OnSize(float width, float height)
{
	Window::OnSize(width, height);

	_arrow->Move(0, height - _arrow->GetHeight() - 1);
	_blankText->Move( 0, _arrow->GetY() - 2 );
	_input->Move( _arrow->GetWidth(), _arrow->GetY()-1 );
	_input->Resize( width - _arrow->GetWidth(), _input->GetHeight() );
}

bool Console::OnFocus(bool focus)
{
	return true;
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
