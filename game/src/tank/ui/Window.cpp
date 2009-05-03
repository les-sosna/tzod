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
	_color = 0xffffffff;

	_manager     = manager;
	_parent      = parent;
	_firstChild  = NULL;
	_lastChild   = NULL;
	_nextSibling = NULL;
	if( _parent )
	{
		if( _prevSibling = _parent->_lastChild )
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

	SetTexture("ui/window");

	_x      = 0;
	_y      = 0;
	_width  = GetTextureWidth();
	_height = GetTextureHeight();
}

Window::Window(Window* parent)
{
	Reg(parent, parent->_manager);

	_frame    = 0;

	SetTexture("ui/window");

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

bool Window::IsCaptured() const
{
	return this == _manager->GetCapture();
}

float Window::GetTextureWidth()  const
{
	return (-1 != _texture) ? g_texman->Get(_texture).pxFrameWidth : 1;
}

float Window::GetTextureHeight() const
{
	return (-1 != _texture) ? g_texman->Get(_texture).pxFrameHeight : 1;
}

void Window::SetTexture(const char *tex)
{
	if( tex )
	{
		_texture = g_texman->FindSprite(tex);
	}
	else
	{
		_texture = -1;
	}
}

unsigned int Window::GetFrameCount() const
{
	return (-1 != _texture) ? (g_texman->Get(_texture).xframes * g_texman->Get(_texture).yframes) : 0;
}

void Window::Draw(float sx, float sy) const
{
	if( !_isVisible )
	{
		return;
	}


	float l = sx + _x;
	float r = l  + _width;
	float t = sy + _y;
	float b = t  + _height;

	if( -1 != _texture )
	{
		const LogicalTexture &lt = g_texman->Get(_texture);
		const FRECT &rt = lt.uvFrames[_frame];
		g_render->TexBind(lt.dev_texture);

		MyVertex *v;


		//
		// draw border
		//

		if( _hasBorder )
		{
			const float pxBorderSize  = 2;
			const float uvBorderWidth = pxBorderSize * lt.uvFrameWidth / lt.pxFrameWidth;
			const float uvBorderHeight = pxBorderSize * lt.uvFrameHeight / lt.pxFrameHeight;

			// left edge
			v = g_render->DrawQuad();
			v[0].color = _color;
			v[0].u = lt.uvLeft - uvBorderWidth;
			v[0].v = lt.uvTop;
			v[0].x = l - pxBorderSize;
			v[0].y = t;
			v[1].color = _color;
			v[1].u = lt.uvLeft;
			v[1].v = lt.uvTop;
			v[1].x = l;
			v[1].y = t;
			v[2].color = _color;
			v[2].u = lt.uvLeft;
			v[2].v = lt.uvBottom;
			v[2].x = l;
			v[2].y = b;
			v[3].color = _color;
			v[3].u = lt.uvLeft - uvBorderWidth;
			v[3].v = lt.uvBottom;
			v[3].x = l - pxBorderSize;
			v[3].y = b;

			// right edge
			v = g_render->DrawQuad();
			v[0].color = _color;
			v[0].u = lt.uvRight;
			v[0].v = lt.uvTop;
			v[0].x = r;
			v[0].y = t;
			v[1].color = _color;
			v[1].u = lt.uvRight + uvBorderWidth;
			v[1].v = lt.uvTop;
			v[1].x = r + pxBorderSize;
			v[1].y = t;
			v[2].color = _color;
			v[2].u = lt.uvRight + uvBorderWidth;
			v[2].v = lt.uvBottom;
			v[2].x = r + pxBorderSize;
			v[2].y = b;
			v[3].color = _color;
			v[3].u = lt.uvRight;
			v[3].v = lt.uvBottom;
			v[3].x = r;
			v[3].y = b;

			// top edge
			v = g_render->DrawQuad();
			v[0].color = _color;
			v[0].u = lt.uvLeft;
			v[0].v = lt.uvTop - uvBorderHeight;
			v[0].x = l;
			v[0].y = t - pxBorderSize;
			v[1].color = _color;
			v[1].u = lt.uvRight;
			v[1].v = lt.uvTop - uvBorderHeight;
			v[1].x = r;
			v[1].y = t - pxBorderSize;
			v[2].color = _color;
			v[2].u = lt.uvRight;
			v[2].v = lt.uvTop;
			v[2].x = r;
			v[2].y = t;
			v[3].color = _color;
			v[3].u = lt.uvLeft;
			v[3].v = lt.uvTop;
			v[3].x = l;
			v[3].y = t;

			// bottom edge
			v = g_render->DrawQuad();
			v[0].color = _color;
			v[0].u = lt.uvLeft;
			v[0].v = lt.uvBottom;
			v[0].x = l;
			v[0].y = b;
			v[1].color = _color;
			v[1].u = lt.uvRight;
			v[1].v = lt.uvBottom;
			v[1].x = r;
			v[1].y = b;
			v[2].color = _color;
			v[2].u = lt.uvRight;
			v[2].v = lt.uvBottom + uvBorderHeight;
			v[2].x = r;
			v[2].y = b + pxBorderSize;
			v[3].color = _color;
			v[3].u = lt.uvLeft;
			v[3].v = lt.uvBottom + uvBorderHeight;
			v[3].x = l;
			v[3].y = b + pxBorderSize;

			// left top corner
			v = g_render->DrawQuad();
			v[0].color = _color;
			v[0].u = lt.uvLeft - uvBorderWidth;
			v[0].v = lt.uvTop - uvBorderHeight;
			v[0].x = l - pxBorderSize;
			v[0].y = t - pxBorderSize;
			v[1].color = _color;
			v[1].u = lt.uvLeft;
			v[1].v = lt.uvTop - uvBorderHeight;
			v[1].x = l;
			v[1].y = t - pxBorderSize;
			v[2].color = _color;
			v[2].u = lt.uvLeft;
			v[2].v = lt.uvTop;
			v[2].x = l;
			v[2].y = t;
			v[3].color = _color;
			v[3].u = lt.uvLeft - uvBorderWidth;
			v[3].v = lt.uvTop;
			v[3].x = l - pxBorderSize;
			v[3].y = t;

			// right top corner
			v = g_render->DrawQuad();
			v[0].color = _color;
			v[0].u = lt.uvRight;
			v[0].v = lt.uvTop - uvBorderHeight;
			v[0].x = r;
			v[0].y = t - pxBorderSize;
			v[1].color = _color;
			v[1].u = lt.uvRight + uvBorderWidth;
			v[1].v = lt.uvTop - uvBorderHeight;
			v[1].x = r + pxBorderSize;
			v[1].y = t - pxBorderSize;
			v[2].color = _color;
			v[2].u = lt.uvRight + uvBorderWidth;
			v[2].v = lt.uvTop;
			v[2].x = r + pxBorderSize;
			v[2].y = t;
			v[3].color = _color;
			v[3].u = lt.uvRight;
			v[3].v = lt.uvTop;
			v[3].x = r;
			v[3].y = t;

			// right bottom corner
			v = g_render->DrawQuad();
			v[0].color = _color;
			v[0].u = lt.uvRight;
			v[0].v = lt.uvBottom;
			v[0].x = r;
			v[0].y = b;
			v[1].color = _color;
			v[1].u = lt.uvRight + uvBorderWidth;
			v[1].v = lt.uvBottom;
			v[1].x = r + pxBorderSize;
			v[1].y = b;
			v[2].color = _color;
			v[2].u = lt.uvRight + uvBorderWidth;
			v[2].v = lt.uvBottom + uvBorderHeight;
			v[2].x = r + pxBorderSize;
			v[2].y = b + pxBorderSize;
			v[3].color = _color;
			v[3].u = lt.uvRight;
			v[3].v = lt.uvBottom + uvBorderHeight;
			v[3].x = r;
			v[3].y = b + pxBorderSize;

			// left bottom corner
			v = g_render->DrawQuad();
			v[0].color = _color;
			v[0].u = lt.uvLeft - uvBorderWidth;
			v[0].v = lt.uvBottom;
			v[0].x = l - pxBorderSize;
			v[0].y = b;
			v[1].color = _color;
			v[1].u = lt.uvLeft;
			v[1].v = lt.uvBottom;
			v[1].x = l;
			v[1].y = b;
			v[2].color = _color;
			v[2].u = lt.uvLeft;
			v[2].v = lt.uvBottom + uvBorderHeight;
			v[2].x = l;
			v[2].y = b + pxBorderSize;
			v[3].color = _color;
			v[3].u = lt.uvLeft - uvBorderWidth;
			v[3].v = lt.uvBottom + uvBorderHeight;
			v[3].x = l - pxBorderSize;
			v[3].y = b + pxBorderSize;
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

	//
	// draw children windows with optional clipping
	//

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

void Window::DrawChildren(float sx, float sy) const
{
	for( Window *w = _firstChild; w; w = w->_nextSibling )
	{
		if( !w->_isTopMost ) // topmost windows are drawn separately
		{
			w->Draw(sx, sy);
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

void Window::SetCapture()
{
	_manager->SetCapture(this);
}

void Window::ReleaseCapture()
{
	_manager->ReleaseCapture(this);
}

void Window::Reset() // called when window is being hidden or disabled
{
	if( GetManager()->GetFocusWnd() ) GetManager()->Unfocus(this);
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
		w->OnEnabledChange(enable, true);
	}
}

void Window::OnVisibleChangeInternal(bool visible, bool inherited)
{
	OnVisibleChange(visible, inherited);
	for( Window *w = _firstChild; w; w = w->_nextSibling )
	{
		w->OnVisibleChange(visible, true);
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

void Window::OnEnabledChange(bool enable, bool inherited)
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
