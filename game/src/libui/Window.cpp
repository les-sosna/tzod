#include "inc/ui/Window.h"
#include "inc/ui/InputContext.h"
#include "inc/ui/GuiManager.h"
#include <video/TextureManager.h>
#include <video/DrawingContext.h>
#include <algorithm>

using namespace UI;

Window::Window(LayoutManager &manager)
  : _manager(manager)
  , _isVisible(true)
  , _isEnabled(true)
  , _isTopMost(false)
  , _isTimeStep(false)
  , _drawBorder(true)
  , _drawBackground(true)
  , _clipChildren(false)
{
}

Window::~Window()
{
	UnlinkAllChildren();
}

void Window::PrepareToUnlink(Window &child)
{
	if (_focusChild.get() == &child)
		_focusChild = nullptr;

	// removes mouse captures if any.
	GetManager().GetInputContext().ResetWindow(child);

	if (child.GetTimeStep())
		GetManager().TimeStepUnregister(child._timeStepReg);
}

void Window::UnlinkAllChildren()
{
	for (auto &child: _children)
	{
		PrepareToUnlink(*child);
	}
	_children.clear();
}

void Window::UnlinkChild(Window &child)
{
	PrepareToUnlink(child);
	_children.erase(
		std::remove_if(std::begin(_children), std::end(_children), [&](auto &which) { return which.get() == &child;} ),
		_children.end());
}

void Window::AddFront(std::shared_ptr<Window> child)
{
	_children.push_back(std::move(child));
}

void Window::AddBack(std::shared_ptr<Window> child)
{
	_children.push_front(std::move(child));
}

FRECT Window::GetChildRect(vec2d size, const Window &child) const
{
	return FRECT{ child._x, child._y, child._x + child._width, child._y + child._height };
}

float Window::GetTextureWidth(TextureManager &texman) const
{
	return (-1 != _texture) ? texman.GetFrameWidth(_texture, _frame) : 1;
}

float Window::GetTextureHeight(TextureManager &texman) const
{
	return (-1 != _texture) ? texman.GetFrameHeight(_texture, _frame) : 1;
}

void Window::SetTexture(TextureManager &texman, const char *tex, bool fitSize)
{
	if( tex )
	{
		_texture = texman.FindSprite(tex);
		if( fitSize )
		{
			Resize(GetTextureWidth(texman), GetTextureHeight(texman));
		}
	}
	else
	{
		_texture = (size_t) -1;
	}
}

void Window::SetTextureStretchMode(StretchMode stretchMode)
{
	_textureStretchMode = stretchMode;
}

unsigned int Window::GetFrameCount() const
{
	return (-1 != _texture) ? GetManager().GetTextureManager().GetFrameCount(_texture) : 0;
}

void Window::Draw(bool hovered, bool focused, bool enabled, vec2d size, InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	assert(_isVisible);

	FRECT dst = {0, 0, size.x, size.y};

	if( -1 != _texture )
	{
		if( _drawBackground )
		{
			float border = _drawBorder ? texman.GetBorderSize(_texture) : 0.f;
			FRECT client = { dst.left + border, dst.top + border, dst.right - border, dst.bottom - border };
			if (_textureStretchMode == StretchMode::Stretch)
			{
				dc.DrawSprite(&client, _texture, _backColor, _frame);
			}
			else
			{
				RectRB clip;
				FRectToRect(&clip, &client);
				dc.PushClippingRect(clip);

				float frameWidth = texman.GetFrameWidth(_texture, _frame);
				float frameHeight = texman.GetFrameHeight(_texture, _frame);

				if (WIDTH(client) * frameHeight > HEIGHT(client) * frameWidth)
				{
					float newHeight = WIDTH(client) / frameWidth * frameHeight;
					client.top = (HEIGHT(client) - newHeight) / 2;
					client.bottom = client.top + newHeight;
				}
				else
				{
					float newWidth = HEIGHT(client) / frameHeight * frameWidth;
					client.left = (WIDTH(client) - newWidth) / 2;
					client.right = client.left + newWidth;
				}

				dc.DrawSprite(&client, _texture, _backColor, _frame);

				dc.PopClippingRect();
			}
		}
		if( _drawBorder )
		{
			dc.DrawBorder(dst, _texture, _borderColor, _frame);
		}
	}
}

void Window::Move(float x, float y)
{
	_x = x;
	_y = y;
}

void Window::Resize(float width, float height)
{
	if( _width != width || _height != height )
	{
		_width  = width;
		_height = height;
		OnSize(width, height);
	}
}

void Window::SetTopMost(bool topmost)
{
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

void Window::SetFocus(std::shared_ptr<Window> child)
{
	assert(!child || _children.end() != std::find(_children.begin(), _children.end(), child));
	_focusChild = child;
}

std::shared_ptr<Window> Window::GetFocus() const
{
	return _focusChild;
}

void Window::OnEnabledChangeInternal(bool enable, bool inherited)
{
	if( enable )
	{
		// enable children last
		if( !inherited )
			_isEnabled = true;
		OnEnabledChange(true, inherited);
		for( auto &w: _children )
		{
			w->OnEnabledChangeInternal(true, true);
		}
	}
	else
	{
		// disable children first
		for (auto &w : _children)
		{
			w->OnEnabledChangeInternal(false, true);
		}
		GetManager().GetInputContext().ResetWindow(*this);
		if( !inherited )
			_isEnabled = false;
		OnEnabledChange(false, inherited);
	}
}

void Window::OnVisibleChangeInternal(bool visible, bool inherited)
{
	if( visible )
	{
		// show children last
		if (!inherited)
			_isVisible = true;

		for (auto &w : _children)
			w->OnVisibleChangeInternal(true, true);
	}
	else
	{
		// hide children first
		for (auto &w : _children)
			w->OnVisibleChangeInternal(false, true);

		GetManager().GetInputContext().ResetWindow(*this);

		if (!inherited)
			_isVisible = false;
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

void Window::SetVisible(bool visible)
{
	if( _isVisible != visible )
	{
		OnVisibleChangeInternal(visible, false);
		assert(_isVisible == visible);
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
// size
//

void Window::OnSize(float width, float height)
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

void Window::OnTimeStep(LayoutManager &manager, float dt)
{
}
