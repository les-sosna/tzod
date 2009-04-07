// Window.cpp

#include "stdafx.h"

#include "Window.h"
#include "GuiManager.h"

#include "video/TextureManager.h"


namespace UI
{
;

///////////////////////////////////////////////////////////////////////////////
// Window class implementation

void Window::Reg(Window* parent, GuiManager* manager)
{
	_manager     = manager;
	_parent      = parent;
	_firstChild  = NULL;
	_lastChild   = NULL;
	_nextSibling = NULL;
	if( _parent )
	{
		if( _prevSibling = _parent->_lastChild )
		{
			_ASSERT(NULL == _prevSibling->_nextSibling);
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

	_isDestroyed  = false;
	_isVisible    = true;
	_isEnabled    = true;
	_isTopMost    = false;
	_isTimeStep   = false;
	_hasBorder    = false;
	_clipChildren = false;

	_manager->Add(this);
	AddRef(); // increment ref counter to allow using Destroy()
}

Window::Window(GuiManager* manager)
{
	Reg(NULL, manager);

	_frame    = 0;

	SetTexture("window");

	_x      = 0;
	_y      = 0;
	_width  = GetTextureWidth();
	_height = GetTextureHeight();
}

Window::Window(Window* parent)
{
	Reg(parent, parent->_manager);

	_frame    = 0;

	SetTexture("window");

	_x      = 0;
	_y      = 0;
	_width  = GetTextureWidth();
	_height = GetTextureHeight();
}

Window::Window(Window* parent, float x, float y, const char *texture)
{
	Reg(parent, parent->_manager);

	_frame    = 0;

	SetTexture(texture);

	_x      = x;
	_y      = y;
	_width  = GetTextureWidth();
	_height = GetTextureHeight();
}

Window::~Window()
{
	_ASSERT( _isDestroyed );
}

void Window::Destroy()
{
	if( !_isDestroyed )
	{
		_isDestroyed = true;

		//
		// remove this window from the manager
		//
		if( IsTopMost()  ) SetTopMost(false);
		if( IsTimeStep() ) SetTimeStep(false);
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
			_ASSERT( this == _prevSibling->_nextSibling );
			_prevSibling->_nextSibling = _nextSibling;
		}

		if( _nextSibling )
		{
			_ASSERT( this == _nextSibling->_prevSibling );
			_nextSibling->_prevSibling = _prevSibling;
		}

		if( _parent )
		{
			if( this == _parent->_firstChild )
			{
				_ASSERT(NULL == _prevSibling);
				_parent->_firstChild = _nextSibling;
			}

			if( this == _parent->_lastChild )
			{
				_ASSERT(NULL == _nextSibling);
				_parent->_lastChild = _prevSibling;
			}

			_parent->Release();
		}

		Release();
	}
}

bool Window::IsCaptured() const
{
	return this == _manager->GetCapture();
}

float Window::GetTextureWidth()  const
{
	return _texture ? g_texman->Get(_texture).pxFrameWidth : 1;
}

float Window::GetTextureHeight() const
{
	return _texture ? g_texman->Get(_texture).pxFrameHeight : 1;
}

void Window::SetTexture(const char *tex)
{
	if( tex )
	{
		_texture = g_texman->FindTexture(tex);
		_color   = g_texman->Get(_texture).color;
	}
	else
	{
		_texture = 0;
		_color   = 0xffffffff;
	}
}

int Window::GetFrameCount() const
{
	return _texture ? (g_texman->Get(_texture).xframes * g_texman->Get(_texture).yframes) : 0;
}

void Window::Draw(float sx, float sy)
{
	if( !_isVisible )
	{
		return;
	}


	float l = sx + _x;
	float r = l  + _width;
	float t = sy + _y;
	float b = t  + _height;

	if( _texture )
	{
		const LogicalTexture &lt = g_texman->Get(_texture);
		const FRECT &rt = lt.uvFrames[_frame];
		g_render->TexBind(lt.dev_texture);

		const float borderWidth = 2;
		const float uvBorder    = borderWidth * lt.uvFrameWidth / lt.pxFrameWidth;


		MyVertex *v;


		//
		// draw border
		//

		if( _hasBorder )
		{
			// left edge

			v = g_render->DrawQuad();
			v[0].color = _color;
			v[0].u = rt.left - uvBorder;
			v[0].v = rt.top;
			v[0].x = l - borderWidth;
			v[0].y = t;
			v[1].color = _color;
			v[1].u = rt.left;
			v[1].v = rt.top;
			v[1].x = l;
			v[1].y = t;
			v[2].color = _color;
			v[2].u = rt.left;
			v[2].v = rt.bottom;
			v[2].x = l;
			v[2].y = b;
			v[3].color = _color;
			v[3].u = rt.left - uvBorder;
			v[3].v = rt.bottom;
			v[3].x = l - borderWidth;
			v[3].y = b;


			// right edge

			v = g_render->DrawQuad();
			v[0].color = _color;
			v[0].u = rt.right;
			v[0].v = rt.top;
			v[0].x = r;
			v[0].y = t;
			v[1].color = _color;
			v[1].u = rt.right + uvBorder;
			v[1].v = rt.top;
			v[1].x = r + borderWidth;
			v[1].y = t;
			v[2].color = _color;
			v[2].u = rt.right + uvBorder;
			v[2].v = rt.bottom;
			v[2].x = r + borderWidth;
			v[2].y = b;
			v[3].color = _color;
			v[3].u = rt.right;
			v[3].v = rt.bottom;
			v[3].x = r;
			v[3].y = b;


			// top edge

			v = g_render->DrawQuad();
			v[0].color = _color;
			v[0].u = rt.left;
			v[0].v = rt.top - uvBorder;
			v[0].x = l;
			v[0].y = t - borderWidth;
			v[1].color = _color;
			v[1].u = rt.right;
			v[1].v = rt.top - uvBorder;
			v[1].x = r;
			v[1].y = t - borderWidth;
			v[2].color = _color;
			v[2].u = rt.right;
			v[2].v = rt.top;
			v[2].x = r;
			v[2].y = t;
			v[3].color = _color;
			v[3].u = rt.left;
			v[3].v = rt.top;
			v[3].x = l;
			v[3].y = t;


			// bottom edge

			v = g_render->DrawQuad();
			v[0].color = _color;
			v[0].u = rt.left;
			v[0].v = rt.bottom;
			v[0].x = l;
			v[0].y = b;
			v[1].color = _color;
			v[1].u = rt.right;
			v[1].v = rt.bottom;
			v[1].x = r;
			v[1].y = b;
			v[2].color = _color;
			v[2].u = rt.right;
			v[2].v = rt.bottom + uvBorder;
			v[2].x = r;
			v[2].y = b + borderWidth;
			v[3].color = _color;
			v[3].u = rt.left;
			v[3].v = rt.bottom + uvBorder;
			v[3].x = l;
			v[3].y = b + borderWidth;


			// left top corner

			v = g_render->DrawQuad();
			v[0].color = _color;
			v[0].u = rt.left - uvBorder;
			v[0].v = rt.top - uvBorder;
			v[0].x = l - borderWidth;
			v[0].y = t - borderWidth;
			v[1].color = _color;
			v[1].u = rt.left;
			v[1].v = rt.top - uvBorder;
			v[1].x = l;
			v[1].y = t - borderWidth;
			v[2].color = _color;
			v[2].u = rt.left;
			v[2].v = rt.top;
			v[2].x = l;
			v[2].y = t;
			v[3].color = _color;
			v[3].u = rt.left - uvBorder;
			v[3].v = rt.top;
			v[3].x = l - borderWidth;
			v[3].y = t;


			// right top corner

			v = g_render->DrawQuad();
			v[0].color = _color;
			v[0].u = rt.right;
			v[0].v = rt.top - uvBorder;
			v[0].x = r;
			v[0].y = t - borderWidth;
			v[1].color = _color;
			v[1].u = rt.right + uvBorder;
			v[1].v = rt.top - uvBorder;
			v[1].x = r + borderWidth;
			v[1].y = t - borderWidth;
			v[2].color = _color;
			v[2].u = rt.right + uvBorder;
			v[2].v = rt.top;
			v[2].x = r + borderWidth;
			v[2].y = t;
			v[3].color = _color;
			v[3].u = rt.right;
			v[3].v = rt.top;
			v[3].x = r;
			v[3].y = t;


			// right bottom corner

			v = g_render->DrawQuad();
			v[0].color = _color;
			v[0].u = rt.right;
			v[0].v = rt.bottom;
			v[0].x = r;
			v[0].y = b;
			v[1].color = _color;
			v[1].u = rt.right + uvBorder;
			v[1].v = rt.bottom;
			v[1].x = r + borderWidth;
			v[1].y = b;
			v[2].color = _color;
			v[2].u = rt.right + uvBorder;
			v[2].v = rt.bottom + uvBorder;
			v[2].x = r + borderWidth;
			v[2].y = b + borderWidth;
			v[3].color = _color;
			v[3].u = rt.right;
			v[3].v = rt.bottom + uvBorder;
			v[3].x = r;
			v[3].y = b + borderWidth;


			// left bottom corner

			v = g_render->DrawQuad();
			v[0].color = _color;
			v[0].u = rt.left - uvBorder;
			v[0].v = rt.bottom;
			v[0].x = l - borderWidth;
			v[0].y = b;
			v[1].color = _color;
			v[1].u = rt.left;
			v[1].v = rt.bottom;
			v[1].x = l;
			v[1].y = b;
			v[2].color = _color;
			v[2].u = rt.left;
			v[2].v = rt.bottom + uvBorder;
			v[2].x = l;
			v[2].y = b + borderWidth;
			v[3].color = _color;
			v[3].u = rt.left - uvBorder;
			v[3].v = rt.bottom + uvBorder;
			v[3].x = l - borderWidth;
			v[3].y = b + borderWidth;
		}

		//
		// draw entire window
		//

		v = g_render->DrawQuad();

		v[0].color = _color;
		v[0].u = rt.left;
		v[0].v = rt.top;
		v[0].x = l;
		v[0].y = t;

		v[1].color = _color;
		v[1].u = rt.right;
		v[1].v = rt.top;
		v[1].x = r;
		v[1].y = t;

		v[2].color = _color;
		v[2].u = rt.right;
		v[2].v = rt.bottom;
		v[2].x = r;
		v[2].y = b;

		v[3].color = _color;
		v[3].u = rt.left;
		v[3].v = rt.bottom;
		v[3].x = l;
		v[3].y = b;
	}

	//---------------------------
	// draw children windows

	if( _clipChildren )
	{
		RECT clip;
		clip.left   = (int) l;
		clip.top    = (int) t;
		clip.right  = (int) r;
		clip.bottom = (int) b;
		_manager->PushClippingRect(clip);
	}

	DrawChildren(sx + _x, sy + _y);

	if( _clipChildren )
	{
		_manager->PopClippingRect();
	}
}

void Window::DrawChildren(float sx, float sy)
{
	for( Window *w = _firstChild; w; w = w->_nextSibling )
	{
		if( !w->_isTopMost ) // topmost windows are drawn separately
			w->Draw(sx, sy);
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
	_ASSERT(_isTopMost != topmost);
	_manager->AddTopMost(this, topmost);
	_isTopMost = topmost;
}

void Window::SetTimeStep(bool enable)
{
	if( enable )
	{
		_ASSERT(!IsDestroyed());
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

bool Window::GetTimeStep() const
{
	return _isTimeStep;
}

void Window::SetCapture()
{
	_manager->SetCapture(this);
}

void Window::ReleaseCapture()
{
	_manager->ReleaseCapture(this);
}

void Window::Enable(bool enable)
{
	if( _isEnabled != enable )
	{
		_isEnabled = enable;
		if( !IsEnabled() )
		{
			if( GetManager()->GetFocusWnd() ) GetManager()->Unfocus(this);
			if( GetManager()->GetHotTrackWnd() ) GetManager()->ResetHotTrackWnd(this);
		}
		OnEnable(enable);
	}
}

void Window::Show(bool show)
{
	if( _isVisible != show )
	{
		_isVisible = show;
		if( !IsVisible() )
		{
			if( GetManager()->GetFocusWnd() ) GetManager()->Unfocus(this);
			if( GetManager()->GetHotTrackWnd() ) GetManager()->ResetHotTrackWnd(this);
		}
		OnShow(show);
	}
}

void Window::BringToFront()
{
	_ASSERT(_parent);
	if( _nextSibling )
	{
		_ASSERT( _parent->_firstChild );
		_ASSERT( this != _parent->_lastChild );


		//
		// unregister
		//

		if( _prevSibling )
		{
			_ASSERT( this == _prevSibling->_nextSibling );
			_prevSibling->_nextSibling = _nextSibling;
		}
		else
		{
			_ASSERT( this == _parent->_firstChild );
			_parent->_firstChild = _nextSibling;
		}

		_ASSERT( this == _nextSibling->_prevSibling );
		_nextSibling->_prevSibling = _prevSibling;
		_nextSibling = NULL;


		//
		// register
		//

		_prevSibling = _parent->_lastChild;
		_ASSERT(_prevSibling);
		_ASSERT(NULL == _prevSibling->_nextSibling);
		_prevSibling->_nextSibling = this;
		_parent->_lastChild = this;
	}
}


void Window::BringToBack()
{
	_ASSERT(_parent);
	if( _prevSibling )
	{
		_ASSERT( _parent->_lastChild );
		_ASSERT( this != _parent->_firstChild );


		//
		// unregister
		//

		if( _nextSibling )
		{
			_ASSERT( this == _nextSibling->_prevSibling );
			_nextSibling->_prevSibling = _prevSibling;
		}
		else
		{
			_ASSERT( this == _parent->_lastChild );
			_parent->_lastChild = _prevSibling;
		}

		_ASSERT( this == _prevSibling->_nextSibling );
		_prevSibling->_nextSibling = _nextSibling;
		_prevSibling = NULL;


		//
		// register
		//

		_nextSibling = _parent->_firstChild;
		_ASSERT(_nextSibling);
		_ASSERT(NULL == _nextSibling->_prevSibling);
		_nextSibling->_prevSibling = this;
		_parent->_firstChild = this;
	}
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

void Window::OnChar(int c)
{
}

void Window::OnRawChar(int c)
{
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

void Window::OnEnable(bool enable)
{
}

bool Window::OnFocus(bool focus)
{
	return false;
}

void Window::OnShow(bool show)
{
}

void Window::OnTimeStep(float dt)
{
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
