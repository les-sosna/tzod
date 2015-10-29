// Window.cpp

#include "inc/ui/Window.h"
#include "inc/ui/GuiManager.h"
#include <video/TextureManager.h>
#include <video/DrawingContext.h>

namespace UI
{

Window* Window::Create(Window *parent)
{
	assert(parent);
	return new Window(parent);
}

///////////////////////////////////////////////////////////////////////////////
// Window class implementation

#ifdef NDEBUG
#define AssertNoDestroy(x)  ((void) 0)
#else
#define AssertNoDestroy(x) NoDestroyHelper __dontdestroyme(x)
#endif

Window::Window(Window *parent, LayoutManager *manager)
    : _resident(new Resident(this))
    , _manager(parent ? parent->GetManager() : *manager)
    , _parent(parent)
    , _firstChild(nullptr)
    , _lastChild(nullptr)
    , _nextSibling(nullptr)
    , _x(0)
    , _y(0)
    , _width(0)
    , _height(0)
    , _backColor(0xffffffff)
    , _borderColor(0xffffffff)
    , _texture(-1)
    , _frame(0)
    , _isVisible(true)
    , _isEnabled(true)
    , _isTopMost(false)
    , _isTimeStep(false)
    , _drawBorder(true)
    , _drawBackground(true)
    , _clipChildren(false)
#ifndef NDEBUG
  , _debugNoDestroy(0)
#endif
{
	if( _parent )
	{
		_prevSibling = _parent->_lastChild;
		if( _prevSibling )
		{
			assert(nullptr == _prevSibling->_nextSibling);
			_prevSibling->_nextSibling = this;
		}
		_parent->_lastChild = this;
		if( !_parent->_firstChild )
			_parent->_firstChild = this;
	}
	else
	{
		_prevSibling = nullptr;
	}
}

Window::~Window()
{
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
			assert(nullptr == _prevSibling);
			_parent->_firstChild = _nextSibling;
		}
		if( this == _parent->_lastChild )
		{
			assert(nullptr == _nextSibling);
			_parent->_lastChild = _prevSibling;
		}
	}
}

void Window::Destroy()
{
	assert(!_debugNoDestroy);
	{
		AssertNoDestroy(this);

		// this removes focus and mouse hover if any.
		// the window don't yet suspect that it's being destroyed
		GetManager().ResetWindow(this);

		// do not call virtual functions after children got destroyed!
		while( Window *c = GetFirstChild() )
		{
			c->Destroy();
		}

		if( _isTopMost )
			GetManager().AddTopMost(this, false);
		if( _isTimeStep )
			GetManager().TimeStepUnregister(_timeStepReg);
	}

	// mark window as dead
	assert(_resident);
	_resident->ptr = nullptr;
	if( 0 == _resident->counter )
	{
		delete _resident;
	}
	_resident = nullptr;

	// do destroy
	delete this;
}

bool Window::Contains(const Window *other) const
{
	while( other )
	{
		if( this == other )
		{
			return true;
		}
		other = other->_parent;
	}
	return false;
}

float Window::GetTextureWidth() const
{
	return (-1 != _texture) ? GetManager().GetTextureManager().GetFrameWidth(_texture, _frame) : 1;
}

float Window::GetTextureHeight() const
{
	return (-1 != _texture) ? GetManager().GetTextureManager().GetFrameHeight(_texture, _frame) : 1;
}

void Window::SetTexture(const char *tex, bool fitSize)
{
	if( tex )
	{
		_texture = GetManager().GetTextureManager().FindSprite(tex);
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
	return (-1 != _texture) ? GetManager().GetTextureManager().GetFrameCount(_texture) : 0;
}

void Window::Draw(DrawingContext &dc, float sx, float sy) const
{
	AssertNoDestroy(this);
	assert(_isVisible);

	//           left     top      right             bottom
	FRECT dst = {sx + _x, sy + _y, sx + _x + _width, sy + _y + _height};

	if( -1 != _texture )
	{
		if( _drawBackground )
		{
			dc.DrawSprite(&dst, _texture, _backColor, _frame);
		}
		if( _drawBorder )
		{
			 dc.DrawBorder(&dst, _texture, _borderColor, _frame);
		}
	}

	//
	// draw children windows with optional clipping
	//

	if( _clipChildren )
	{
		RectRB clip;
		clip.left   = (int) dst.left;
		clip.top    = (int) dst.top;
		clip.right  = (int) dst.right;
		clip.bottom = (int) dst.bottom;
		dc.PushClippingRect(clip);
	}

	DrawChildren(dc, sx + _x, sy + _y);

	if( _clipChildren )
	{
		dc.PopClippingRect();
	}
}

void Window::DrawChildren(DrawingContext &dc, float sx, float sy) const
{
	AssertNoDestroy(this);

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
	if( _x != x || _y != y )
	{
		_x = x;
		_y = y;
		OnMove(x, y);
	}
}

void Window::Resize(float width, float height)
{
	AssertNoDestroy(this);
	if( _width != width || _height != height )
	{
		_width  = width;
		_height = height;
		OnSize(width, height);
		for( Window *w = _firstChild; w; w = w->_nextSibling )
		{
			w->OnParentSize(width, height);
		}
	}
}

void Window::SetTopMost(bool topmost)
{
	assert(_isTopMost != topmost);
	GetManager().AddTopMost(this, topmost);
	_isTopMost = topmost;
}

void Window::SetTimeStep(bool enable)
{
	if( enable != _isTimeStep )
	{
		if( _isTimeStep )
			GetManager().TimeStepUnregister(_timeStepReg);
		else
			_timeStepReg = GetManager().TimeStepRegister(this);
		_isTimeStep = enable;
	}
}

void Window::OnEnabledChangeInternal(bool enable, bool inherited)
{
	AssertNoDestroy(this);
	if( enable )
	{
		// enable children last
		if( !inherited ) _isEnabled = true;
		OnEnabledChange(true, inherited);
		for( Window *w = _firstChild; w; w = w->_nextSibling )
		{
			w->OnEnabledChangeInternal(true, true);
		}
	}
	else
	{
		// disable children first
		for( Window *w = _firstChild; w; w = w->_nextSibling )
		{
			w->OnEnabledChangeInternal(false, true);
		}
		GetManager().ResetWindow(this);
		if( !inherited ) _isEnabled = false;
		OnEnabledChange(false, inherited);
	}
}

void Window::OnVisibleChangeInternal(bool visible, bool inherited)
{
	AssertNoDestroy(this);
	if( visible )
	{
		// show children last
		if( !inherited ) _isVisible = true;
		OnVisibleChange(true, inherited);
		for( Window *w = _firstChild; w; w = w->_nextSibling )
		{
			w->OnVisibleChangeInternal(true, true);
		}
	}
	else
	{
		// hide children first
		for( Window *w = _firstChild; w; w = w->_nextSibling )
		{
			w->OnVisibleChangeInternal(false, true);
		}
		GetManager().ResetWindow(this);
		if( !inherited ) _isVisible = false;
		OnVisibleChange(false, inherited);
	}
}

void Window::SetEnabled(bool enable)
{
	if( _isEnabled != enable )
	{
		OnEnabledChangeInternal(enable, false);
		assert(_isEnabled == enable);
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
		OnVisibleChangeInternal(visible, false);
		assert(_isVisible == visible);
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
		_nextSibling = nullptr;


		//
		// register
		//

		_prevSibling = _parent->_lastChild;
		assert(_prevSibling);
		assert(nullptr == _prevSibling->_nextSibling);
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
		_prevSibling = nullptr;


		//
		// register
		//

		_nextSibling = _parent->_firstChild;
		assert(_nextSibling);
		assert(nullptr == _nextSibling->_prevSibling);
		_nextSibling->_prevSibling = this;
		_parent->_firstChild = this;
	}
}


const std::string& Window::GetText() const
{
	return _text;
}

void Window::SetText(const std::string &text)
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

bool Window::OnKeyPressed(Key key)
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
