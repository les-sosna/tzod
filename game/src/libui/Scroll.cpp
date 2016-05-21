#include "inc/ui/Scroll.h"
#include "inc/ui/Button.h"
#include "inc/ui/GuiManager.h"
#include <algorithm>
#include <cmath>

namespace UI
{

ScrollBarBase::ScrollBarBase(LayoutManager &manager)
    : Window(manager)
    , _tmpBoxPos(-1)
    , _pos(0)
    , _lineSize(0.1f)
    , _pageSize(0)
    , _documentSize(1.0f)
    , _showButtons(true)
{
	_btnBox = std::make_shared<ImageButton>(manager);
	AddFront(_btnBox);
	_btnUpLeft = std::make_shared<ImageButton>(manager);
	AddFront(_btnUpLeft);
	_btnDownRight = std::make_shared<ImageButton>(manager);
	AddFront(_btnDownRight);

	_btnUpLeft->eventClick = std::bind(&ScrollBarBase::OnUpLeft, this);
	_btnDownRight->eventClick = std::bind(&ScrollBarBase::OnDownRight, this);

	_btnBox->eventMouseUp = std::bind(&ScrollBarBase::OnBoxMouseUp, this, std::placeholders::_1, std::placeholders::_2);
	_btnBox->eventMouseDown = std::bind(&ScrollBarBase::OnBoxMouseDown, this, std::placeholders::_1, std::placeholders::_2);
	_btnBox->eventMouseMove = std::bind(&ScrollBarBase::OnBoxMouseMove, this, std::placeholders::_1, std::placeholders::_2);
	_btnBox->SetDrawBorder(true);

	SetDrawBorder(true);
	SetShowButtons(true);
}

void ScrollBarBase::SetShowButtons(bool showButtons)
{
	_showButtons = showButtons;
	_btnUpLeft->SetVisible(showButtons);
	_btnDownRight->SetVisible(showButtons);
	SetPos(GetPos());  // to update scroll box position
}

bool ScrollBarBase::GetShowButtons() const
{
	return _showButtons;
}

void ScrollBarBase::SetPos(float pos)
{
	_pos = std::max(.0f, std::min(_documentSize - _pageSize, pos));
}

float ScrollBarBase::GetPos() const
{
	return _pos;
}

void ScrollBarBase::SetPageSize(float ps)
{
	_pageSize = ps;
	OnLimitsChanged();
	SetPos(GetPos()); // update scroll box size and position
}

float ScrollBarBase::GetPageSize() const
{
	return _pageSize;
}

void ScrollBarBase::SetDocumentSize(float limit)
{
	_documentSize = limit;
	OnLimitsChanged();
	SetPos(GetPos()); // update scroll box position
}

float ScrollBarBase::GetDocumentSize() const
{
	return _documentSize;
}

void ScrollBarBase::SetLineSize(float ls)
{
	_lineSize = ls;
}

float ScrollBarBase::GetLineSize() const
{
	return _lineSize;
}

void ScrollBarBase::SetElementTextures(TextureManager &texman, const char *slider, const char *upleft, const char *downright)
{
	_btnBox->SetTexture(texman, slider, true);
	_btnUpLeft->SetTexture(texman, upleft, true);
	_btnDownRight->SetTexture(texman, downright, true);

	_btnBox->Move((GetWidth() - _btnBox->GetWidth()) / 2, (GetHeight() - _btnBox->GetHeight()) / 2);
	SetPos(GetPos()); // update scroll position
}

void ScrollBarBase::OnEnabledChange(bool enable, bool inherited)
{
	SetFrame(enable ? 0 : 1);
}

void ScrollBarBase::OnBoxMouseDown(float x, float y)
{
	_tmpBoxPos = Select(x, y);
}

void ScrollBarBase::OnBoxMouseUp(float x, float y)
{
	_tmpBoxPos = -1;
}

void ScrollBarBase::OnBoxMouseMove(float x, float y)
{
	if( _tmpBoxPos < 0 )
		return;
	float pos = Select(_btnBox->GetX() + x, _btnBox->GetY() + y) - _tmpBoxPos;
	if( _showButtons )
	{
		pos -= Select(_btnUpLeft->GetWidth(), _btnUpLeft->GetHeight());
	}
	pos /= GetScrollPaneLength() - Select(_btnBox->GetWidth(), _btnBox->GetHeight());
	SetPos(pos * (_documentSize - _pageSize));
	if( eventScroll )
		eventScroll(GetPos());
}

void ScrollBarBase::OnUpLeft()
{
	SetPos(GetPos() - _lineSize);
	if( eventScroll )
		eventScroll(GetPos());
}

void ScrollBarBase::OnDownRight()
{
	SetPos(GetPos() + _lineSize);
	if( eventScroll )
		eventScroll(GetPos());
}

void ScrollBarBase::OnLimitsChanged()
{
	bool needScroll = _documentSize > _pageSize;
	_btnBox->SetVisible(needScroll);
	_btnUpLeft->SetEnabled(needScroll);
	_btnDownRight->SetEnabled(needScroll);
	if( !needScroll )
	{
		SetPos(0);
	}
}

void ScrollBarBase::OnSize(float width, float height)
{
	_btnDownRight->Move( Select(width - _btnDownRight->GetWidth(), _btnDownRight->GetX()),
	                     Select(_btnDownRight->GetY(), height - _btnDownRight->GetHeight()) );
	SetPos(GetPos());  // to update scroll box position
}

float ScrollBarBase::GetScrollPaneLength() const
{
	float result = Select(GetWidth(), GetHeight());
	if( _showButtons )
	{
		result -= Select( _btnDownRight->GetWidth() + _btnUpLeft->GetWidth(),
		                 _btnDownRight->GetHeight() + _btnUpLeft->GetHeight() );
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////////

ScrollBarVertical::ScrollBarVertical(LayoutManager &manager, TextureManager &texman)
  : ScrollBarBase(manager)
{
	_btnBox->SetTexture(texman, "ui/scroll_vert", true);
	_btnUpLeft->SetTexture(texman, "ui/scroll_up", true);
	_btnDownRight->SetTexture(texman, "ui/scroll_down", true);
	SetTexture(texman, "ui/scroll_back_vert", true);
}

void ScrollBarVertical::SetSize(float size)
{
	Resize(GetWidth(), size);
}

float ScrollBarVertical::GetSize() const
{
	return GetHeight();
}

void ScrollBarVertical::SetPos(float pos)
{
	ScrollBarBase::SetPos(pos);

	float mult = GetShowButtons() ? 1.0f : 0.0f;
	_btnBox->Resize(_btnBox->GetWidth(),
		std::max(GetScrollPaneLength() * GetPageSize() / GetDocumentSize(), _btnBox->GetTextureHeight(GetManager().GetTextureManager())));
	_btnBox->Move(_btnBox->GetX(), floor(_btnUpLeft->GetHeight() * mult + (GetHeight() - _btnBox->GetHeight()
		- (_btnDownRight->GetHeight() + _btnUpLeft->GetHeight()) * mult ) * GetPos() / (GetDocumentSize() - GetPageSize()) + 0.5f));
}

///////////////////////////////////////////////////////////////////////////////

ScrollBarHorizontal::ScrollBarHorizontal(LayoutManager &manager, TextureManager &texman)
  : ScrollBarBase(manager)
{
	_btnBox->SetTexture(texman, "ui/scroll_hor", true);
	_btnUpLeft->SetTexture(texman, "ui/scroll_left", true);
	_btnDownRight->SetTexture(texman, "ui/scroll_right", true);
	SetTexture(texman, "ui/scroll_back_hor", true);
}

void ScrollBarHorizontal::SetSize(float size)
{
	Resize(size, GetHeight());
}

float ScrollBarHorizontal::GetSize() const
{
	return GetWidth();
}

void ScrollBarHorizontal::SetPos(float pos)
{
	ScrollBarBase::SetPos(pos);

	float mult = GetShowButtons() ? 1.0f : 0.0f;
	_btnBox->Resize(std::max(GetScrollPaneLength() * GetPageSize() / GetDocumentSize(), _btnBox->GetTextureWidth(GetManager().GetTextureManager())),
		_btnBox->GetHeight());
	_btnBox->Move(floor(_btnUpLeft->GetWidth() * mult + (GetWidth() - _btnBox->GetWidth()
		- (_btnUpLeft->GetWidth() + _btnDownRight->GetWidth()) * mult) * GetPos() / (GetDocumentSize() - GetPageSize()) + 0.5f), _btnBox->GetY());
}

} // end of namespace UI
