#include "inc/ui/Scroll.h"
#include "inc/ui/Button.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/LayoutContext.h"
#include <algorithm>
#include <cmath>

static const float MIN_THUMB_SIZE = 10.f;

namespace UI
{

ScrollBarBase::ScrollBarBase(LayoutManager &manager, TextureManager &texman)
    : Rectangle(manager)
    , _tmpBoxPos(-1)
    , _pos(0)
    , _lineSize(0.1f)
    , _pageSize(0)
    , _documentSize(1.0f)
    , _showButtons(true)
{
	_btnBox = std::make_shared<Button>(manager, texman);
	AddFront(_btnBox);
	_btnUpLeft = std::make_shared<Button>(manager, texman);
	AddFront(_btnUpLeft);
	_btnDownRight = std::make_shared<Button>(manager, texman);
	AddFront(_btnDownRight);

	_btnUpLeft->eventClick = std::bind(&ScrollBarBase::OnUpLeft, this);
	_btnDownRight->eventClick = std::bind(&ScrollBarBase::OnDownRight, this);

	_btnBox->eventMouseUp = std::bind(&ScrollBarBase::OnBoxMouseUp, this, std::placeholders::_1, std::placeholders::_2);
	_btnBox->eventMouseDown = std::bind(&ScrollBarBase::OnBoxMouseDown, this, std::placeholders::_1, std::placeholders::_2);
	_btnBox->eventMouseMove = std::bind(&ScrollBarBase::OnBoxMouseMove, this, std::placeholders::_1, std::placeholders::_2);

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
	_btnBox->SetBackground(texman, slider, true);
	_btnUpLeft->SetBackground(texman, upleft, true);
	_btnDownRight->SetBackground(texman, downright, true);

	_btnBox->Move((GetWidth() - _btnBox->GetWidth()) / 2, (GetHeight() - _btnBox->GetHeight()) / 2);
	SetPos(GetPos()); // update scroll position
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
	float pos = Select(_boxPos.x + x, _boxPos.y + y) - _tmpBoxPos;
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

void ScrollBarBase::Draw(const StateContext &sc, const LayoutContext &lc, const InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	const_cast<ScrollBarBase*>(this)->SetFrame(lc.GetEnabled() ? 0 : 1);
	Rectangle::Draw(sc, lc, ic, dc, texman);
}

FRECT ScrollBarBase::GetChildRect(TextureManager &texman, const LayoutContext &lc, const StateContext &sc, const Window &child) const
{
	float scale = lc.GetScale();
	vec2d size = lc.GetPixelSize();

	if (_btnBox.get() == &child)
	{
		return CanvasLayout(_boxPos, child.GetSize(), scale);
	}
	else if (_btnDownRight.get() == &child)
	{
		return CanvasLayout(size / scale - child.GetSize(), child.GetSize(), scale);
	}

	return Rectangle::GetChildRect(texman, lc, sc, child);
}

///////////////////////////////////////////////////////////////////////////////

ScrollBarVertical::ScrollBarVertical(LayoutManager &manager, TextureManager &texman)
  : ScrollBarBase(manager, texman)
{
	_btnBox->SetBackground(texman, "ui/scroll_vert", true);
	_btnUpLeft->SetBackground(texman, "ui/scroll_up", true);
	_btnDownRight->SetBackground(texman, "ui/scroll_down", true);
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
	_btnBox->Resize(_btnBox->GetWidth(), std::max(GetScrollPaneLength() * GetPageSize() / GetDocumentSize(), MIN_THUMB_SIZE));
	_boxPos.y = _btnUpLeft->GetHeight() * mult + (GetHeight() - _btnBox->GetHeight()
		- (_btnDownRight->GetHeight() + _btnUpLeft->GetHeight()) * mult ) * GetPos() / (GetDocumentSize() - GetPageSize());
}

///////////////////////////////////////////////////////////////////////////////

ScrollBarHorizontal::ScrollBarHorizontal(LayoutManager &manager, TextureManager &texman)
  : ScrollBarBase(manager, texman)
{
	_btnBox->SetBackground(texman, "ui/scroll_hor", true);
	_btnUpLeft->SetBackground(texman, "ui/scroll_left", true);
	_btnDownRight->SetBackground(texman, "ui/scroll_right", true);
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
	_btnBox->Resize(std::max(GetScrollPaneLength() * GetPageSize() / GetDocumentSize(), MIN_THUMB_SIZE), _btnBox->GetHeight());
	_boxPos.x = _btnUpLeft->GetWidth() * mult + (GetWidth() - _btnBox->GetWidth()
		- (_btnUpLeft->GetWidth() + _btnDownRight->GetWidth()) * mult) * GetPos() / (GetDocumentSize() - GetPageSize());
}

} // end of namespace UI
