// Console.cpp

#include "Console.h"
#include "Edit.h"
#include "Scroll.h"
#include "ConsoleBuffer.h"
#include "GuiManager.h"

#include "video/TextureManager.h"
#include "video/DrawingContext.h"

#include <GLFW/glfw3.h>

#include <algorithm>

namespace UI
{


ConsoleHistoryDefault::ConsoleHistoryDefault(size_t maxSize)
  : _maxSize(maxSize)
{
}

void ConsoleHistoryDefault::Enter(const std::string &str)
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

const std::string& ConsoleHistoryDefault::GetItem(size_t index) const
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
  , _cmdIndex(0)
  , _font(GetManager().GetTextureManager().FindSprite("font_small"))
  , _buf(NULL)
  , _history(NULL)
  , _echo(true)
  , _autoScroll(true)
{
	_input = Edit::Create(this, 0, 0, 0);
	_scroll = ScrollBarVertical::Create(this, 0, 0, 0);
	_scroll->eventScroll = std::bind(&Console::OnScroll, this, std::placeholders::_1);
	SetTexture("ui/console", false);
	SetDrawBorder(true);
	SetTimeStep(true); // FIXME: workaround
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
	_cmdIndex = _history ? _history->GetItemCount() : 0;
}

void Console::SetBuffer(ConsoleBuffer *buf)
{
	_buf = buf;
	_scroll->SetDocumentSize(_buf ? (float) _buf->GetLineCount() + _scroll->GetPageSize() : 0);
}

void Console::SetEcho(bool echo)
{
	_echo = echo;
}

bool Console::OnChar(int c)
{
	GetManager().SetFocusWnd(_input);
	return true;
}

bool Console::OnRawChar(int c)
{
	switch(c)
	{
	case GLFW_KEY_UP:
//		if( GetAsyncKeyState(VK_CONTROL) & 0x8000 ) // FIXME: workaround
//		{
//			_scroll->SetPos(_scroll->GetPos() - 1);
//			_autoScroll = _scroll->GetPos() + _scroll->GetPageSize() >= _scroll->GetDocumentSize();
//		}
//		else
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
	case GLFW_KEY_DOWN:
//		if( GetAsyncKeyState(VK_CONTROL) & 0x8000 ) // FIXME: workaround
//		{
//			_scroll->SetPos(_scroll->GetPos() + 1);
//			_autoScroll = _scroll->GetPos() + _scroll->GetPageSize() >= _scroll->GetDocumentSize();
//		}
//		else
        if( _history )
		{
			++_cmdIndex;
			if( _cmdIndex < _history->GetItemCount() )
			{
				_input->SetText(_history->GetItem(_cmdIndex));
			}
			else
			{
				_input->SetText(std::string());
				_cmdIndex = _history->GetItemCount();
			}
		}
		break;
	case GLFW_KEY_ENTER:
	{
		const std::string &cmd = _input->GetText();
		if( cmd.empty() )
		{
			_buf->WriteLine(0, std::string(">"));
		}
		else
		{
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
				eventOnSendCommand(cmd.c_str());
			_input->SetText(std::string());
		}
		_scroll->SetPos(_scroll->GetDocumentSize());
		_autoScroll = true;
		break;
	}
	case GLFW_KEY_PAGE_UP:
		_scroll->SetPos(_scroll->GetPos() - _scroll->GetPageSize());
		_autoScroll = _scroll->GetPos() + _scroll->GetPageSize() >= _scroll->GetDocumentSize();
		break;
	case GLFW_KEY_PAGE_DOWN:
		_scroll->SetPos(_scroll->GetPos() + _scroll->GetPageSize());
		_autoScroll = _scroll->GetPos() + _scroll->GetPageSize() >= _scroll->GetDocumentSize();
		break;
//	case GLFW_KEY_HOME:
//		break;
//	case GLFW_KEY_END:
//		break;
	case GLFW_KEY_ESCAPE:
		if( _input->GetText().empty() )
			return false;
		_input->SetText(std::string());
		break;
	case GLFW_KEY_TAB:
		if( eventOnRequestCompleteCommand )
		{
			std::string result;
			int pos = _input->GetSelEnd();
			if( eventOnRequestCompleteCommand(_input->GetText(), pos, result) )
			{
				_input->SetText(result);
				_input->SetSel(pos, pos);
			}
			_scroll->SetPos(_scroll->GetDocumentSize());
			_autoScroll = true;
		}
		break;
	default:
		return false;
	}
	return true;
}

bool Console::OnMouseWheel(float x, float y, float z)
{
	_scroll->SetPos(_scroll->GetPos() - z * 3);
	_autoScroll = _scroll->GetPos() + _scroll->GetPageSize() >= _scroll->GetDocumentSize();
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

void Console::OnTimeStep(float dt)
{
	// FIXME: workaround
	_scroll->SetDocumentSize(_buf ? (float) _buf->GetLineCount() + _scroll->GetPageSize() - 1 : 0);
	if( _autoScroll )
		_scroll->SetPos(_scroll->GetDocumentSize());
}

void Console::DrawChildren(DrawingContext &dc, float sx, float sy) const
{
	if( _buf )
	{
		_buf->Lock();

		float h = GetManager().GetTextureManager().GetFrameHeight(_font, 0);
		size_t visibleLineCount = size_t(_input->GetY() / h);
		size_t scroll  = std::min(size_t(_scroll->GetDocumentSize() - _scroll->GetPos() - _scroll->GetPageSize()), _buf->GetLineCount());
		size_t lineMax = _buf->GetLineCount() - scroll;
		size_t count   = std::min(lineMax, visibleLineCount);

		float y = sy - fmod(_input->GetY(), h) + (float) (visibleLineCount - count) * h;

		for( size_t line = lineMax - count; line < lineMax; ++line )
		{
			unsigned int sev = _buf->GetSeverity(line);
			SpriteColor color = sev < _colors.size() ? _colors[sev] : 0xffffffff;
			dc.DrawBitmapText(sx + 4, y, _font, color, _buf->GetLine(line));
			y += h;
		}

		_buf->Unlock();

		if( _autoScroll )
		{
			// FIXME: magic number
			dc.DrawBitmapText(sx + _scroll->GetX() - 2, sy + _input->GetY(), _font, 0x7f7f7f7f, "auto", alignTextRB);
		}
	}
	Window::DrawChildren(dc, sx, sy);
}

void Console::OnSize(float width, float height)
{
	_input->Move(0, height - _input->GetHeight());
	_input->Resize(width, _input->GetHeight());
	_scroll->Move(width - _scroll->GetWidth(), 0);
	_scroll->Resize(_scroll->GetWidth(), height - _input->GetHeight());
	_scroll->SetPageSize(_input->GetY() / GetManager().GetTextureManager().GetFrameHeight(_font, 0));
	_scroll->SetDocumentSize(_buf ? (float) _buf->GetLineCount() + _scroll->GetPageSize() : 0);
}

bool Console::OnFocus(bool focus)
{
	return true;
}

void Console::OnScroll(float pos)
{
	_autoScroll = pos + _scroll->GetPageSize() >= _scroll->GetDocumentSize();
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
