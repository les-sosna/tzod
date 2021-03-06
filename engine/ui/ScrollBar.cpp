#include "inc/ui/ScrollBar.h"
#include "inc/ui/Button.h"
#include "inc/ui/DataSource.h"
#include "inc/ui/LayoutContext.h"
#include <algorithm>
#include <cmath>

static constexpr float MIN_THUMB_SIZE = 10.f;

using namespace UI;

ScrollBarBase::ScrollBarBase()
	: _tmpBoxPos(-1)
	, _pos(0)
	, _lineSize(0.1f)
	, _pageSize(0)
	, _documentSize(1.0f)
	, _showButtons(true)
{
	_background = std::make_shared<Rectangle>();
	_background->SetDrawBorder(true);
	AddFront(_background);

	_btnBox = std::make_shared<Button>();
	AddFront(_btnBox);
	_btnUpLeft = std::make_shared<Button>();
	AddFront(_btnUpLeft);
	_btnDownRight = std::make_shared<Button>();
	AddFront(_btnDownRight);

	_btnUpLeft->eventClick = std::bind(&ScrollBarBase::OnUpLeft, this);
	_btnDownRight->eventClick = std::bind(&ScrollBarBase::OnDownRight, this);

	SetShowButtons(true);
}

void ScrollBarBase::SetShowButtons(bool showButtons)
{
	_showButtons = showButtons;
	_btnUpLeft->SetVisible(showButtons);
	_btnDownRight->SetVisible(showButtons);
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
	_btnBox->SetBackground(slider);
	_btnBox->AlignToBackground(texman);
	_btnUpLeft->SetBackground(upleft);
	_btnUpLeft->AlignToBackground(texman);
	_btnDownRight->SetBackground(downright);
	_btnDownRight->AlignToBackground(texman);
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
/*	if( _tmpBoxPos < 0 )
		return;
	float pos = Select(_boxPos.x + x, _boxPos.y + y) - _tmpBoxPos;
	if( _showButtons )
	{
		pos -= Select(_btnUpLeft->GetWidth(), _btnUpLeft->GetHeight());
	}
	pos /= GetScrollPaneLength() - Select(_btnBox->GetWidth(), _btnBox->GetHeight());
	SetPos(pos * (_documentSize - _pageSize));
	if( eventScroll )
		eventScroll(GetPos());*/
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
	if( !needScroll )
	{
		SetPos(0);
	}
}

float ScrollBarBase::GetScrollPaneLength(const LayoutContext &lc) const
{
	float result = Select(lc.GetPixelSize().x / lc.GetScaleCombined(), lc.GetPixelSize().y / lc.GetScaleCombined());
	if( _showButtons )
	{
		result -= Select( _btnDownRight->GetWidth() + _btnUpLeft->GetWidth(),
		                 _btnDownRight->GetHeight() + _btnUpLeft->GetHeight() );
	}
	return result;
}

WindowLayout ScrollBarBase::GetChildLayout(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	float scale = lc.GetScaleCombined();
	vec2d size = lc.GetPixelSize();

	if (_background.get() == &child)
	{
		return WindowLayout{ MakeRectWH(size), 1, true };
	}
	if (_btnDownRight.get() == &child)
	{
		return WindowLayout{ CanvasLayout(size / scale - child.GetSize(), child.GetSize(), scale), 1, _documentSize > _pageSize };
	}
	if (_btnUpLeft.get() == &child)
	{
		return WindowLayout{ CanvasLayout(vec2d{}, child.GetSize(), lc.GetScaleCombined()), 1, _documentSize > _pageSize };
	}
	assert(false);
	return {};
}

///////////////////////////////////////////////////////////////////////////////

ScrollBarVertical::ScrollBarVertical(TextureManager &texman)
{
	_btnBox->SetBackground("ui/scroll_vert");
	_btnBox->AlignToBackground(texman);
	_btnUpLeft->SetBackground("ui/scroll_up");
	_btnUpLeft->AlignToBackground(texman);
	_btnDownRight->SetBackground("ui/scroll_down");
	_btnDownRight->AlignToBackground(texman);
	_background->SetTexture("ui/scroll_back_vert");
	Resize(_background->GetTextureWidth(texman), _background->GetTextureHeight(texman));
}

WindowLayout ScrollBarVertical::GetChildLayout(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	if (_btnBox.get() == &child)
	{
		float height = lc.GetPixelSize().y / lc.GetScaleCombined();
		float mult = GetShowButtons() ? 1.0f : 0.0f;
		vec2d thumbSize = { _btnBox->GetWidth(), std::max(GetScrollPaneLength(lc) * GetPageSize() / GetDocumentSize(), MIN_THUMB_SIZE) };
		vec2d thumbOffset = { 0, _btnUpLeft->GetHeight() * mult + (height - thumbSize.y
			- (_btnDownRight->GetHeight() + _btnUpLeft->GetHeight()) * mult) * GetPos() / (GetDocumentSize() - GetPageSize()) };

		return WindowLayout{ CanvasLayout(thumbOffset, thumbSize, lc.GetScaleCombined()), 1, true };
	}

	return ScrollBarBase::GetChildLayout(texman, lc, dc, child);
}

///////////////////////////////////////////////////////////////////////////////

ScrollBarHorizontal::ScrollBarHorizontal(TextureManager &texman)
{
	_btnBox->SetBackground("ui/scroll_hor");
	_btnBox->AlignToBackground(texman);
	_btnUpLeft->SetBackground("ui/scroll_left");
	_btnUpLeft->AlignToBackground(texman);
	_btnDownRight->SetBackground("ui/scroll_right");
	_btnDownRight->AlignToBackground(texman);
	_background->SetTexture("ui/scroll_back_hor");
	Resize(_background->GetTextureWidth(texman), _background->GetTextureHeight(texman));
}

WindowLayout ScrollBarHorizontal::GetChildLayout(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	if (_btnBox.get() == &child)
	{
		float width = lc.GetPixelSize().x / lc.GetScaleCombined();
		float mult = GetShowButtons() ? 1.0f : 0.0f;
		vec2d thumbSize = { std::max(GetScrollPaneLength(lc) * GetPageSize() / GetDocumentSize(), MIN_THUMB_SIZE), _btnBox->GetHeight() };
		vec2d thumbOffset = { _btnUpLeft->GetWidth() * mult + (width - thumbSize.x
			- (_btnUpLeft->GetWidth() + _btnDownRight->GetWidth()) * mult) * GetPos() / (GetDocumentSize() - GetPageSize()), 0 };

		return WindowLayout{ CanvasLayout(thumbOffset, thumbSize, lc.GetScaleCombined()), 1, true };
	}

	return ScrollBarBase::GetChildLayout(texman, lc, dc, child);
}
