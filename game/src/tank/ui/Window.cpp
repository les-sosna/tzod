// Window.cpp

#include "stdafx.h"

#include "Window.h"
#include "GuiManager.h"

#include "video/TextureManager.h"


namespace UI
{

Window* Window::Create(Window *parent)
{
	assert(parent);
	return new Window(parent);
}

///////////////////////////////////////////////////////////////////////////////
// Window class implementation

Window::Window(Window *parent, LayoutManager *manager)
  : _x(0)
  , _y(0)
  , _width(0)
  , _height(0)
  , _backColor(0xffffffff)
  , _borderColor(0xffffffff)
  , _frame(0)
  , _manager(parent ? parent->GetManager() : manager)
  , _parent(parent)
  , _firstChild(NULL)
  , _lastChild(NULL)
  , _nextSibling(NULL)
  , _isDestroyed(false)
  , _isVisible(true)
  , _isEnabled(true)
  , _isTopMost(false)
  , _isTimeStep(false)
  , _drawBorder(true)
  , _drawBackground(true)
  , _clipChildren(false)
{
	if( _parent )
	{
		_prevSibling = _parent->_lastChild;
		if( _prevSibling )
		{
			assert(NULL == _prevSibling->_nextSibling);
			_prevSibling->_nextSibling = this;
		}
		_parent->_lastChild = this;
		if( !_parent->_firstChild )
			_parent->_firstChild = this;
		_parent->AddRef();
	}
	else
	{
		_prevSibling = NULL;
	}

	_manager->Add(this);
	AddRef(); // increment ref counter to allow using Destroy()

	SetTexture("ui/window", true);
}

Window::~Window()
{
	assert( _isDestroyed );
}

void Window::Destroy()
{
	if( !_isDestroyed )
	{
		_isDestroyed = true;

		//
		// remove this window from the manager
		//
		if( GetTopMost()  ) SetTopMost(false);
		if( GetTimeStep() ) SetTimeStep(false);
		_manager->Remove(this);


		// destroy all children
		if( Window *w = _firstChild )
		{
			w->AddRef();
			for( ;; )
			{
				Window *tmp = w;

				w = w->_nextSibling;
				if( w )
					w->AddRef();

				tmp->Destroy();
				tmp->Release();

				if( !w )
					break;
			}
		}


		//
		// destroy it self
		//

		if( _prevSibling )
		{
			assert(this == _prevSibling->_nextSibling);
			_prevSibling->_nextSibling = _nextSibling;
		}

		if( _nextSibling )
		{
			assert(this == _nextSibling->_prevSibling);
			_nextSibling->_prevSibling = _prevSibling;
		}

		if( _parent )
		{
			if( this == _parent->_firstChild )
			{
				assert(NULL == _prevSibling);
				_parent->_firstChild = _nextSibling;
			}

			if( this == _parent->_lastChild )
			{
				assert(NULL == _nextSibling);
				_parent->_lastChild = _prevSibling;
			}

			_parent->Release();
		}

		Release();
	}
}

float Window::GetTextureWidth() const
{
	return (-1 != _texture) ? GetManager()->GetTextureManager()->GetFrameWidth(_texture, _frame) : 1;
}

float Window::GetTextureHeight() const
{
	return (-1 != _texture) ? GetManager()->GetTextureManager()->GetFrameHeight(_texture, _frame) : 1;
}

void Window::SetTexture(const char *tex, bool fitSize)
{
	if( tex )
	{
		_texture = GetManager()->GetTextureManager()->FindSprite(tex);
		if( fitSize )
		{
			Resize(GetTextureWidth(), GetTextureHeight());
		}
	}
	else
	{
		_texture = (size_t) -1;
	}
}

unsigned int Window::GetFrameCount() const
{
	return (-1 != _texture) ? GetManager()->GetTextureManager()->GetFrameCount(_texture) : 0;
}

void Window::Draw(const DrawingContext *dc, float sx, float sy) const
{
	assert(_isVisible);

	//           left     top      right             bottom
	FRECT dst = {sx + _x, sy + _y, sx + _x + _width, sy + _y + _height};

	if( -1 != _texture )
	{
		if( _drawBackground )
		{
			dc->DrawSprite(&dst, _texture, _backColor, _frame);
		}
		if( _drawBorder )
		{
			 dc->DrawBorder(&dst, _texture, _borderColor, _frame);
		}
	}

	//
	// draw children windows with optional clipping
	//

	if( _clipChildren )
	{
		RECT clip;
		clip.left   = (int) dst.left;
		clip.top    = (int) dst.top;
		clip.right  = (int) dst.right;
		clip.bottom = (int) dst.bottom;
		dc->PushClippingRect(clip);
	}

	DrawChildren(dc, sx + _x, sy + _y);

	if( _clipChildren )
	{
		dc->PopClippingRect();
	}
}

void Window::DrawChildren(const DrawingContext *dc, float sx, float sy) const
{
	for( Window *w = _firstChild; w; w = w->_nextSibling )
	{
		// topmost windows are drawn separately
		if( !w->_isTopMost && w->_isVisible )
		{
			w->Draw(dc, sx, sy);
		}
	}
}

void Window::Move(float x, float y)
{
	_x = x;
	_y = y;
	OnMove(x, y);
}

void Window::Resize(float width, float height)
{
	_width  = width;
	_height = height;
	OnSize(width, height);
	for( Window *w = _firstChild; w; w = w->_nextSibling )
	{
		w->OnParentSize(width, height);
	}
}

void Window::SetTopMost(bool topmost)
{
	assert(_isTopMost != topmost);
	_manager->AddTopMost(this, topmost);
	_isTopMost = topmost;
}

void Window::SetTimeStep(bool enable)
{
	if( enable )
	{
		assert(!IsDestroyed());
		if( !_isTimeStep )
			_timeStepReg = GetManager()->TimeStepRegister(this);
	}
	else
	{
		if( _isTimeStep )
			GetManager()->TimeStepUnregister(_timeStepReg);
	}
	_isTimeStep = enable;
}

void Window::Reset() // called when window is being hidden or disabled
{
	if( GetManager()->GetFocusWnd() ) GetManager()->ResetFocus(this);
	if( GetManager()->GetHotTrackWnd() ) GetManager()->ResetHotTrackWnd(this);

	for( Window *w = _firstChild; w; w = w->_nextSibling )
	{
		w->Reset();
	}
}

void Window::OnEnabledChangeInternal(bool enable, bool inherited)
{
	OnEnabledChange(enable, inherited);
	for( Window *w = _firstChild; w; w = w->_nextSibling )
	{
		w->OnEnabledChangeInternal(enable, true);
	}
}

void Window::OnVisibleChangeInternal(bool visible, bool inherited)
{
	OnVisibleChange(visible, inherited);
	for( Window *w = _firstChild; w; w = w->_nextSibling )
	{
		w->OnVisibleChangeInternal(visible, true);
	}
}

void Window::SetEnabled(bool enable)
{
	if( _isEnabled != enable )
	{
		_isEnabled = enable;
		if( !enable ) Reset();
		OnEnabledChangeInternal(enable, false);
	}
}

bool Window::GetEnabled() const
{
	return _isEnabled && (GetParent() ? GetParent()->GetEnabled() : true);
}

void Window::SetVisible(bool visible)
{
	if( _isVisible != visible )
	{
		_isVisible = visible;
		if( !visible ) Reset();
		OnVisibleChangeInternal(visible, false);
	}
}

bool Window::GetVisible() const
{
	return _isVisible && (GetParent() ? GetParent()->GetVisible() : true);
}

void Window::BringToFront()
{
	assert(_parent);
	if( _nextSibling )
	{
		assert( _parent->_firstChild );
		assert( this != _parent->_lastChild );


		//
		// unregister
		//

		if( _prevSibling )
		{
			assert( this == _prevSibling->_nextSibling );
			_prevSibling->_nextSibling = _nextSibling;
		}
		else
		{
			assert( this == _parent->_firstChild );
			_parent->_firstChild = _nextSibling;
		}

		assert( this == _nextSibling->_prevSibling );
		_nextSibling->_prevSibling = _prevSibling;
		_nextSibling = NULL;


		//
		// register
		//

		_prevSibling = _parent->_lastChild;
		assert(_prevSibling);
		assert(NULL == _prevSibling->_nextSibling);
		_prevSibling->_nextSibling = this;
		_parent->_lastChild = this;
	}
}


void Window::BringToBack()
{
	assert(_parent);
	if( _prevSibling )
	{
		assert( _parent->_lastChild );
		assert( this != _parent->_firstChild );


		//
		// unregister
		//

		if( _nextSibling )
		{
			assert( this == _nextSibling->_prevSibling );
			_nextSibling->_prevSibling = _prevSibling;
		}
		else
		{
			assert( this == _parent->_lastChild );
			_parent->_lastChild = _prevSibling;
		}

		assert( this == _prevSibling->_nextSibling );
		_prevSibling->_nextSibling = _nextSibling;
		_prevSibling = NULL;


		//
		// register
		//

		_nextSibling = _parent->_firstChild;
		assert(_nextSibling);
		assert(NULL == _nextSibling->_prevSibling);
		_nextSibling->_prevSibling = this;
		_parent->_firstChild = this;
	}
}


const string_t& Window::GetText() const
{
	return _text;
}

void Window::SetText(const string_t &text)
{
	_text.assign(text);
	OnTextChange();
}




//
// mouse handlers
//

bool Window::OnMouseDown(float x, float y, int button)
{
	return false;
}

bool Window::OnMouseUp  (float x, float y, int button)
{
	return false;
}

bool Window::OnMouseMove(float x, float y)
{
	return false;
}

bool Window::OnMouseEnter(float x, float y)
{
	return false;
}

bool Window::OnMouseLeave()
{
	return false;
}

bool Window::OnMouseWheel(float x, float y, float z)
{
	return false;
}


//
// keyboard handlers
//

bool Window::OnChar(int c)
{
	return false;
}

bool Window::OnRawChar(int c)
{
	return false;
}


//
// size & position
//

void Window::OnMove(float x, float y)
{
}

void Window::OnSize(float width, float height)
{
}

void Window::OnParentSize(float width, float height)
{
}


//
// other
//

void Window::OnEnabledChange(bool enable, bool inherited)
{
}

void Window::OnTextChange()
{
}

bool Window::OnFocus(bool focus)
{
	return false;
}

void Window::OnVisibleChange(bool visible, bool inherited)
{
}

void Window::OnTimeStep(float dt)
{
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
