#include "inc/ui/Console.h"
#include "inc/ui/Edit.h"
#include "inc/ui/EditableText.h"
#include "inc/ui/Scroll.h"
#include "inc/ui/ConsoleBuffer.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/Keys.h"
#include "inc/ui/LayoutContext.h"
#include <video/TextureManager.h>
#include <video/RenderContext.h>
#include <algorithm>

using namespace UI;

ConsoleHistoryDefault::ConsoleHistoryDefault(size_t maxSize)
  : _maxSize(maxSize)
{
}

void ConsoleHistoryDefault::Enter(std::string str)
{
    _buf.push_back(std::move(str));
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

std::shared_ptr<Console> Console::Create(Window *parent, TextureManager &texman, float x, float y, float w, float h, ConsoleBuffer *buf)
{
	auto res = std::make_shared<Console>(parent->GetManager(), texman);
	res->Move(x, y);
	res->Resize(w, h);
	res->SetBuffer(buf);
	parent->AddFront(res);
	return res;
}

Console::Console(LayoutManager &manager, TextureManager &texman)
  : Rectangle(manager)
  , _cmdIndex(0)
  , _font(texman.FindSprite("font_small"))
  , _buf(nullptr)
  , _history(nullptr)
  , _echo(true)
  , _autoScroll(true)
{
	_input = std::make_shared<Edit>(manager, texman);
	AddFront(_input);
	SetFocus(_input);
	_scroll = std::make_shared<ScrollBarVertical>(manager, texman);
	_scroll->eventScroll = std::bind(&Console::OnScrollBar, this, std::placeholders::_1);
	AddFront(_scroll);
	SetTexture(texman, "ui/console", false);
	SetDrawBorder(true);
	SetTimeStep(true); // FIXME: workaround
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

bool Console::OnKeyPressed(InputContext &ic, Key key)
{
	switch(key)
	{
	case Key::Up:
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
				_input->GetEditable()->SetText(_history->GetItem(_cmdIndex));
			}
		}
		break;
	case Key::Down:
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
				_input->GetEditable()->SetText(_history->GetItem(_cmdIndex));
			}
			else
			{
				_input->GetEditable()->SetText(std::string());
				_cmdIndex = _history->GetItemCount();
			}
		}
		break;
	case Key::Enter:
	{
		const std::string &cmd = _input->GetEditable()->GetText();
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
			_input->GetEditable()->SetText(std::string());
		}
		_scroll->SetPos(_scroll->GetDocumentSize());
		_autoScroll = true;
		break;
	}
	case Key::PageUp:
		_scroll->SetPos(_scroll->GetPos() - _scroll->GetPageSize());
		_autoScroll = _scroll->GetPos() + _scroll->GetPageSize() >= _scroll->GetDocumentSize();
		break;
	case Key::PageDown:
		_scroll->SetPos(_scroll->GetPos() + _scroll->GetPageSize());
		_autoScroll = _scroll->GetPos() + _scroll->GetPageSize() >= _scroll->GetDocumentSize();
		break;
//	case Key::Home:
//		break;
//	case Key::End:
//		break;
	case Key::Escape:
		if( _input->GetEditable()->GetText().empty() )
			return false;
		_input->GetEditable()->SetText(std::string());
		break;
	case Key::Tab:
		if( eventOnRequestCompleteCommand )
		{
			std::string result;
			int pos = _input->GetEditable()->GetSelEnd();
			if( eventOnRequestCompleteCommand(_input->GetEditable()->GetText(), pos, result) )
			{
				_input->GetEditable()->SetText(result);
				_input->GetEditable()->SetSel(pos, pos);
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

void Console::OnScroll(TextureManager &texman, const InputContext &ic, const LayoutContext &lc, const DataContext &dc, vec2d pointerPosition, vec2d scrollOffset)
{
	_scroll->SetPos(_scroll->GetPos() - scrollOffset.y * 3);
	_autoScroll = _scroll->GetPos() + _scroll->GetPageSize() >= _scroll->GetDocumentSize();
}

void Console::OnTimeStep(LayoutManager &manager, float dt)
{
	// FIXME: workaround
	_scroll->SetLineSize(1);
	_scroll->SetPageSize(20); // textAreaHeight / GetManager().GetTextureManager().GetFrameHeight(_font, 0));
	_scroll->SetDocumentSize(_buf ? (float) _buf->GetLineCount() + _scroll->GetPageSize() - 1 : 0);
	if( _autoScroll )
		_scroll->SetPos(_scroll->GetDocumentSize());
}

void Console::Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman) const
{
	Rectangle::Draw(dc, sc, lc, ic, rc, texman);

	if( _buf )
	{
		_buf->Lock();

		FRECT inputRect = GetChildRect(texman, lc, dc, *_input);
		float textAreaHeight = inputRect.top;

		float h = std::floor(texman.GetFrameHeight(_font, 0) * lc.GetScale());
		size_t visibleLineCount = size_t(textAreaHeight / h);
		size_t scroll  = std::min(size_t(_scroll->GetDocumentSize() - _scroll->GetPos() - _scroll->GetPageSize()), _buf->GetLineCount());
		size_t lineMax = _buf->GetLineCount() - scroll;
		size_t count   = std::min(lineMax, visibleLineCount);

		float y = -fmod(textAreaHeight, h) + (float) (visibleLineCount - count) * h;

		for( size_t line = lineMax - count; line < lineMax; ++line )
		{
			unsigned int sev = _buf->GetSeverity(line);
			SpriteColor color = sev < _colors.size() ? _colors[sev] : 0xffffffff;
			rc.DrawBitmapText(vec2d{ 4, y }, lc.GetScale(), _font, color, _buf->GetLine(line));
			y += h;
		}

		_buf->Unlock();
	}
}

FRECT Console::GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	if (_input.get() == &child)
	{
		float inputHeight = _input->GetContentSize(texman, dc, lc.GetScale()).y;
		return MakeRectRB(vec2d{0, lc.GetPixelSize().y - inputHeight}, lc.GetPixelSize());
	}
	if (_scroll.get() == &child)
	{
		float scrollWidth = std::floor(_scroll->GetWidth() * lc.GetScale());
		return MakeRectRB(vec2d{lc.GetPixelSize().x - scrollWidth}, lc.GetPixelSize());
	}
	return Rectangle::GetChildRect(texman, lc, dc, child);
}

void Console::OnScrollBar(float pos)
{
	_autoScroll = pos + _scroll->GetPageSize() >= _scroll->GetDocumentSize();
}

