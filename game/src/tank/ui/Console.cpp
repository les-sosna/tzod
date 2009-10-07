// Console.cpp

#include "stdafx.h"

#include "Console.h"
#include "Edit.h"
#include "ConsoleBuffer.h"

#include "video/TextureManager.h"
#include "GuiManager.h"


namespace UI
{


ConsoleHistoryDefault::ConsoleHistoryDefault(size_t maxSize)
  : _maxSize(maxSize)
{
}

void ConsoleHistoryDefault::Enter(const string_t &str)
{
	_buf.push_back(str);
	if( _buf.size() > _maxSize )
	{
		_buf.pop_front();
	}
}

size_t ConsoleHistoryDefault::GetItemCount() const
{
	return _buf.size();
}

const string_t& ConsoleHistoryDefault::GetItem(size_t index) const
{
	return _buf[index];
}

///////////////////////////////////////////////////////////////////////////////

Console* Console::Create(Window *parent, float x, float y, float w, float h, ConsoleBuffer *buf)
{
	Console *res = new Console(parent);
	res->Move(x, y);
	res->Resize(w, h);
	res->SetBuffer(buf);
	return res;
}

Console::Console(Window *parent)
  : Window(parent)
  , _buf(NULL)
  , _history(NULL)
  , _echo(true)
  , _font(GetManager()->GetTextureManager()->FindSprite("font_small"))
  , _scrollBack(0)
  , _cmdIndex(0)
{
	SetTexture("ui/console", false);
	SetDrawBorder(true);
	_input = Edit::Create(this, 0, 0, 0);
}

float Console::GetInputHeight() const
{
	return _input->GetHeight();
}

void Console::SetColors(const SpriteColor *colors, size_t count)
{
	_colors.assign(colors, colors + count);
}

void Console::SetHistory(IConsoleHistory *history)
{
	_history = history;
	_cmdIndex = _history->GetItemCount();
}

void Console::SetBuffer(ConsoleBuffer *buf)
{
	_buf = buf;
}

void Console::SetEcho(bool echo)
{
	_echo = echo;
}

bool Console::OnChar(int c)
{
	GetManager()->SetFocusWnd(_input);
	return true;
}

bool Console::OnRawChar(int c)
{
	switch(c)
	{
	case VK_UP:
		if( _history )
		{
			_cmdIndex = std::min(_cmdIndex, _history->GetItemCount());
			if( _cmdIndex > 0 )
			{
				--_cmdIndex;
				_input->SetText(_history->GetItem(_cmdIndex));
			}
		}
		break;
	case VK_DOWN:
		if( _history )
		{
			++_cmdIndex;
			if( _cmdIndex < _history->GetItemCount() )
			{
				_input->SetText(_history->GetItem(_cmdIndex));
			}
			else
			{
				_input->SetText(string_t());
				_cmdIndex = _history->GetItemCount();
			}
		}
		break;
	case VK_RETURN:
	{
		const string_t &cmd = _input->GetText();
		if( cmd.empty() )
		{
			_buf->WriteLine(0, string_t(">"));
		}
		else
		{
			_scrollBack = 0;
			if( _history )
			{
				if( _history->GetItemCount() == 0 || cmd != _history->GetItem(_history->GetItemCount() - 1) )
				{
					_history->Enter(cmd);
				}
				_cmdIndex = _history->GetItemCount();
			}

			if( _echo )
			{
				_buf->Format(0) << "> " << cmd;       // echo to the console
			}
			if( eventOnSendCommand )
				INVOKE(eventOnSendCommand) (cmd.c_str());
			_input->SetText(string_t());
		}
		break;
	}
	case VK_PRIOR:
		_scrollBack = std::min(_scrollBack + 1, _buf->GetLineCount() - 1);
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
		if( _input->GetText().empty() )
			return false;
		_input->SetText(string_t());
		break;
	case VK_TAB:
		if( eventOnRequestCompleteCommand )
		{
			string_t result;
			int pos = _input->GetSelEnd();
			if( INVOKE(eventOnRequestCompleteCommand)(_input->GetText(), pos, result) )
			{
				_input->SetText(result);
				_input->SetSel(pos, pos);
			}
		}
		break;
	default:
		return false;
	}
	return true;
}

bool Console::OnMouseWheel(float x, float y, float z)
{
	if( z > 0 )
	{
		_scrollBack = std::min(_scrollBack + int(z * 3), _buf->GetLineCount() - 1);
	}
	else
	{
		size_t dz = std::min((signed) _scrollBack, int(-z * 3));
		_scrollBack -= dz;
	}
	return true;
}

bool Console::OnMouseDown(float x, float y, int button)
{
	return true;
}

bool Console::OnMouseUp(float x, float y, int button)
{
	return true;
}

bool Console::OnMouseMove(float x, float y)
{
	return true;
}

void Console::DrawChildren(const DrawingContext *dc, float sx, float sy) const
{
	Window::DrawChildren(dc, sx, sy);

	_buf->Lock();

	float h = dc->GetFrameHeight(_font, 0);
	size_t visibleLineCount = size_t(_input->GetY() / h);
	size_t scroll = std::min(_scrollBack, _buf->GetLineCount());
	size_t count  = std::min(_buf->GetLineCount() - scroll, visibleLineCount);

	sy -= fmodf(_input->GetY(), h);
	sy += (float) (visibleLineCount - count) * h;

	for( size_t line = _buf->GetLineCount() - count - scroll; line < _buf->GetLineCount() - scroll; ++line )
	{
		unsigned int sev = _buf->GetSeverity(line);
		SpriteColor color = sev < _colors.size() ? _colors[sev] : 0xffffffff;
		dc->DrawBitmapText(sx + 4, sy, _font, color, _buf->GetLine(line));
		sy += h;
	}

	_buf->Unlock();
}

void Console::OnSize(float width, float height)
{
	_input->Move(0, height - _input->GetHeight());
	_input->Resize(width, _input->GetHeight());
}

bool Console::OnFocus(bool focus)
{
	return true;
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
