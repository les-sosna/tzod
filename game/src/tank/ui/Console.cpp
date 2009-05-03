// Console.cpp

#include "stdafx.h"

#include "Console.h"
#include "Text.h"
#include "Edit.h"

#include "video/TextureManager.h"
#include "core/Console.h"
#include "config/Config.h"

#include "GuiManager.h"


namespace UI
{
///////////////////////////////////////////////////////////////////////////////


Console::Console(Window *parent, float x, float y, float w, float h, ConsoleBuffer *buf)
  : Window(parent, x, y, "ui/window")
  , _buf(buf)
  , _font(g_texman->FindSprite("font_small"))
{
	SetBorder(true);

	_arrow = new Text(this, 0, 0, ">", alignTextLT);

	_input = new Edit(this, 0, 0, 0);
	_input->SetTexture(NULL);

	_scrollBack = 0;
	_cmdIndex   = g_conf->con_history->GetSize();

	_echo = true;

	Resize(w, h);
}

Console::~Console()
{
}

void Console::SetEcho(bool echo)
{
	_echo = echo;
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
		if( _cmdIndex > g_conf->con_history->GetSize() )
		{
			_cmdIndex = g_conf->con_history->GetSize();
		}
		if( _cmdIndex > 0 )
		{
			_input->SetText(g_conf->con_history->GetStr(--_cmdIndex, "")->Get());
		}
		break;
	case VK_DOWN:
		++_cmdIndex;
		if( _cmdIndex < g_conf->con_history->GetSize() )
		{
			_input->SetText(g_conf->con_history->GetStr(_cmdIndex, "")->Get());
		}
		else
		{
			_input->SetText("");
			_cmdIndex = g_conf->con_history->GetSize();
		}
		break;
	case VK_RETURN:
	{
		const string_t &cmd = _input->GetText();
		if( cmd.empty() )
		{
			_buf->printf(">\n");
		}
		else
		{
			_scrollBack = 0;

			if( g_conf->con_history->GetSize() == 0 ||
				cmd != g_conf->con_history->GetStr(g_conf->con_history->GetSize()-1, "")->Get() )
			{
				g_conf->con_history->PushBack(ConfVar::typeString)->AsStr()->Set(cmd);
				while( g_conf->con_history->GetSize() > (unsigned) g_conf->con_maxhistory->GetInt() )
					g_conf->con_history->PopFront();
			}
			_cmdIndex = g_conf->con_history->GetSize();
			if( _echo )
				_buf->printf("> %s\n", cmd.c_str());       // echo to the console
			if( eventOnSendCommand )
				INVOKE(eventOnSendCommand) (cmd);          // send the command
			_input->SetText("");                           // erase input field
		}
		break;
	}
	case VK_PRIOR:
		_scrollBack = __min(_scrollBack + 1, _buf->GetLineCount() - 1);
		break;
	case VK_NEXT:
		if( _scrollBack > 0 ) --_scrollBack;
		break;
	case VK_HOME:
		_scrollBack = _buf->GetLineCount() - 1;
		break;
	case VK_END:
		_scrollBack = 0;
		break;
	case VK_ESCAPE:
		if( !_input->GetText().empty() )
		{
			_input->SetText("");
		}
		else
		{
			GetParent()->OnRawChar(c);
		}
		break;
	case VK_TAB:
		if( eventOnRequestCompleteCommand )
		{
			string_t result;
			bool status = INVOKE(eventOnRequestCompleteCommand)
				(_input->GetText().substr(0, _input->GetSelEnd()), result);
			if( status )
			{
				int end = _input->GetSelEnd();
				_input->SetText(_input->GetText().substr(0, end) + result + _input->GetText().substr(end));
				_input->SetSel(end + result.length(), end + result.length());
			}
		}
		break;
	default:
		GetParent()->OnRawChar(c);
	}
}

bool Console::OnMouseWheel(float x, float y, float z)
{
	if( z > 0 )
	{
		_scrollBack = __min(_scrollBack + int(z * 3), _buf->GetLineCount() - 1);
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

void Console::DrawChildren(float sx, float sy) const
{
	Window::DrawChildren(sx, sy);
	sy += _arrow->GetY() - 2;
	float h = g_texman->Get(_font).pxFrameHeight;
	size_t visibleLineCount = (size_t) (GetHeight() / h);
	size_t scroll = __min(_scrollBack, _buf->GetLineCount());
	size_t count  = __min( _buf->GetLineCount() - scroll, visibleLineCount );
	for( size_t i = 0; i < count; ++i )
	{
		g_texman->DrawBitmapText(_font, _buf->GetLine(_buf->GetLineCount() - scroll - i - 1),
			0xffffffff, sx + 4, sy -= h);
	}
}

void Console::OnSize(float width, float height)
{
	Window::OnSize(width, height);

	_arrow->Move(0, height - _arrow->GetHeight() - 1);
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
