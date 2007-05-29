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
	return _texture ? g_texman->get(_texture).frame_width : 1;
}

float Window::GetTextureHeight() const
{
	return _texture ? g_texman->get(_texture).frame_height : 1;
}

void Window::SetTexture(const char *tex)
{
	if( tex )
	{
		_texture = g_texman->FindTexture(tex);
		_color   = g_texman->get(_texture).color;
	}
	else
	{
		_texture = 0;
		_color   = 0xffffffff;
	}
}

int Window::GetFrameCount() const
{
	return _texture ? (g_texman->get(_texture).xframes * g_texman->get(_texture).yframes) : 0;
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
		g_texman->bind(_texture);
		const LogicalTexture &lt = g_texman->get(_texture);

		float tex_left   = (lt.left+lt.frame_width*(float)(_frame%lt.xframes))*lt.pixel_width;
		float tex_right  = tex_left+lt.frame_width*lt.pixel_width;
		float tex_top    = (lt.top+lt.frame_height*(float)(_frame/lt.xframes))*lt.pixel_height;
		float tex_bottom = tex_top+lt.frame_height*lt.pixel_height;

		const float borderWidth = 2;
		const float borderTex   = borderWidth*lt.pixel_width;


		MyVertex *v;


		//
		// draw border
		//

		if( _hasBorder )
		{
			// left edge

			v = g_render->DrawQuad();
			v[0].color = _color;
			v[0].u = tex_left - borderTex;
			v[0].v = tex_top;
			v[0].x = l - borderWidth;
			v[0].y = t;
			v[1].color = _color;
			v[1].u = tex_left;
			v[1].v = tex_top;
			v[1].x = l;
			v[1].y = t;
			v[2].color = _color;
			v[2].u = tex_left;
			v[2].v = tex_bottom;
			v[2].x = l;
			v[2].y = b;
			v[3].color = _color;
			v[3].u = tex_left - borderTex;
			v[3].v = tex_bottom;
			v[3].x = l - borderWidth;
			v[3].y = b;


			// right edge

			v = g_render->DrawQuad();
			v[0].color = _color;
			v[0].u = tex_right;
			v[0].v = tex_top;
			v[0].x = r;
			v[0].y = t;
			v[1].color = _color;
			v[1].u = tex_right + borderTex;
			v[1].v = tex_top;
			v[1].x = r + borderWidth;
			v[1].y = t;
			v[2].color = _color;
			v[2].u = tex_right + borderTex;
			v[2].v = tex_bottom;
			v[2].x = r + borderWidth;
			v[2].y = b;
			v[3].color = _color;
			v[3].u = tex_right;
			v[3].v = tex_bottom;
			v[3].x = r;
			v[3].y = b;


			// top edge

			v = g_render->DrawQuad();
			v[0].color = _color;
			v[0].u = tex_left;
			v[0].v = tex_top - borderTex;
			v[0].x = l;
			v[0].y = t - borderWidth;
			v[1].color = _color;
			v[1].u = tex_right;
			v[1].v = tex_top - borderTex;
			v[1].x = r;
			v[1].y = t - borderWidth;
			v[2].color = _color;
			v[2].u = tex_right;
			v[2].v = tex_top;
			v[2].x = r;
			v[2].y = t;
			v[3].color = _color;
			v[3].u = tex_left;
			v[3].v = tex_top;
			v[3].x = l;
			v[3].y = t;


			// bottom edge

			v = g_render->DrawQuad();
			v[0].color = _color;
			v[0].u = tex_left;
			v[0].v = tex_bottom;
			v[0].x = l;
			v[0].y = b;
			v[1].color = _color;
			v[1].u = tex_right;
			v[1].v = tex_bottom;
			v[1].x = r;
			v[1].y = b;
			v[2].color = _color;
			v[2].u = tex_right;
			v[2].v = tex_bottom + borderTex;
			v[2].x = r;
			v[2].y = b + borderWidth;
			v[3].color = _color;
			v[3].u = tex_left;
			v[3].v = tex_bottom + borderTex;
			v[3].x = l;
			v[3].y = b + borderWidth;


			// left top corner

			v = g_render->DrawQuad();
			v[0].color = _color;
			v[0].u = tex_left - borderTex;
			v[0].v = tex_top - borderTex;
			v[0].x = l - borderWidth;
			v[0].y = t - borderWidth;
			v[1].color = _color;
			v[1].u = tex_left;
			v[1].v = tex_top - borderTex;
			v[1].x = l;
			v[1].y = t - borderWidth;
			v[2].color = _color;
			v[2].u = tex_left;
			v[2].v = tex_top;
			v[2].x = l;
			v[2].y = t;
			v[3].color = _color;
			v[3].u = tex_left - borderTex;
			v[3].v = tex_top;
			v[3].x = l - borderWidth;
			v[3].y = t;


			// right top corner

			v = g_render->DrawQuad();
			v[0].color = _color;
			v[0].u = tex_right;
			v[0].v = tex_top - borderTex;
			v[0].x = r;
			v[0].y = t - borderWidth;
			v[1].color = _color;
			v[1].u = tex_right + borderTex;
			v[1].v = tex_top - borderTex;
			v[1].x = r + borderWidth;
			v[1].y = t - borderWidth;
			v[2].color = _color;
			v[2].u = tex_right + borderTex;
			v[2].v = tex_top;
			v[2].x = r + borderWidth;
			v[2].y = t;
			v[3].color = _color;
			v[3].u = tex_right;
			v[3].v = tex_top;
			v[3].x = r;
			v[3].y = t;


			// right bottom corner

			v = g_render->DrawQuad();
			v[0].color = _color;
			v[0].u = tex_right;
			v[0].v = tex_bottom;
			v[0].x = r;
			v[0].y = b;
			v[1].color = _color;
			v[1].u = tex_right + borderTex;
			v[1].v = tex_bottom;
			v[1].x = r + borderWidth;
			v[1].y = b;
			v[2].color = _color;
			v[2].u = tex_right + borderTex;
			v[2].v = tex_bottom + borderTex;
			v[2].x = r + borderWidth;
			v[2].y = b + borderWidth;
			v[3].color = _color;
			v[3].u = tex_right;
			v[3].v = tex_bottom + borderTex;
			v[3].x = r;
			v[3].y = b + borderWidth;


			// left bottom corner

			v = g_render->DrawQuad();
			v[0].color = _color;
			v[0].u = tex_left - borderTex;
			v[0].v = tex_bottom;
			v[0].x = l - borderWidth;
			v[0].y = b;
			v[1].color = _color;
			v[1].u = tex_left;
			v[1].v = tex_bottom;
			v[1].x = l;
			v[1].y = b;
			v[2].color = _color;
			v[2].u = tex_left;
			v[2].v = tex_bottom + borderTex;
			v[2].x = l;
			v[2].y = b + borderWidth;
			v[3].color = _color;
			v[3].u = tex_left - borderTex;
			v[3].v = tex_bottom + borderTex;
			v[3].x = l - borderWidth;
			v[3].y = b + borderWidth;
		}

		//
		// draw entire window
		//

		v = g_render->DrawQuad();

		v[0].color = _color;
		v[0].u = tex_left;
		v[0].v = tex_top;
		v[0].x = l;
		v[0].y = t;

		v[1].color = _color;
		v[1].u = tex_right;
		v[1].v = tex_top;
		v[1].x = r;
		v[1].y = t;

		v[2].color = _color;
		v[2].u = tex_right;
		v[2].v = tex_bottom;
		v[2].x = r;
		v[2].y = b;

		v[3].color = _color;
		v[3].u = tex_left;
		v[3].v = tex_bottom;
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

void Window::SetCapture()
{
	_manager->SetCapture(this);
}

void Window::ReleaseCapture()
{
	_manager->ReleaseCapture(this);
}

void Window::Show(bool show)
{
	_isVisible = show;
	OnShow(show);
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
