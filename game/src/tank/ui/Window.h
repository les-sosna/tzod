// Window.h

#pragma once

#include "Base.h"

#include "core/PtrList.h"
#include "core/Delegate.h"

class DrawingContext;

namespace UI
{
	// forward declaration
	class LayoutManager;

///////////////////////////////////////////////////////////////////////////////

class Window
{
	friend class WindowWeakPtr;
	struct Resident
	{
		unsigned int counter;
		Window *ptr;
		Resident(Window *p) : counter(0), ptr(p) {}
	};
	Resident *_resident;

	friend class LayoutManager;
	LayoutManager *_manager;

	Window* _parent;
	Window* _firstChild;
	Window* _lastChild;
	Window* _prevSibling;
	Window* _nextSibling;


	PtrList<Window>::iterator _timeStepReg;


	//
	// size and position
	//

	float _x;
	float _y;
	float _width;
	float _height;


	//
	// attributes
	//

	string_t     _text;

	SpriteColor  _backColor;
	SpriteColor  _borderColor;
	size_t       _texture;
	unsigned int _frame;

	struct
	{
		bool _isVisible      : 1;
		bool _isEnabled      : 1;
		bool _isTopMost      : 1;
		bool _isTimeStep     : 1;
		bool _drawBorder     : 1;
		bool _drawBackground : 1;
		bool _clipChildren   : 1;
	};

#ifdef DEBUG
	unsigned int _debugNoDestroy;
	class NoDestroyHelper
	{
	public:
		NoDestroyHelper(Window *wnd) : _var(wnd->_debugNoDestroy) { ++_var; }
		~NoDestroyHelper() { --_var; }
	private:
		unsigned int &_var;
	};
#endif

protected:
	unsigned int GetFrameCount() const;
	unsigned int GetFrame() const { return _frame; }
	void SetFrame(unsigned int n) { _frame = n; }

	void OnEnabledChangeInternal(bool enable, bool inherited);
	void OnVisibleChangeInternal(bool visible, bool inherited);

protected:
	Window(Window *parent, LayoutManager *manager = NULL);
	virtual ~Window(); // delete via Destroy() only

public:
	static Window* Create(Window *parent);
	void Destroy();

	Window* GetParent()      const { return _parent;      }
	Window* GetPrevSibling() const { return _prevSibling; }
	Window* GetNextSibling() const { return _nextSibling; }
	Window* GetFirstChild()  const { return _firstChild;  }
	Window* GetLastChild()   const { return _lastChild;   }
	LayoutManager* GetManager() const { return _manager;  }

	bool Contains(const Window *other) const;


	//
	// Appearance
	//

	void SetBackColor(SpriteColor color)   { _backColor = color; }
	SpriteColor GetBackColor() const       { return _backColor;  }

	void SetBorderColor(SpriteColor color) { _borderColor = color; }
	SpriteColor GetBorderColor() const     { return _borderColor;  }

	void SetDrawBorder(bool enable)        { _drawBorder = enable; }
	bool GetDrawBorder() const             { return _drawBorder;   }

	void SetDrawBackground(bool enable)    { _drawBackground = enable; }
	bool GetDrawBackground() const         { return _drawBackground;   }

	void SetTexture(const char *tex, bool fitSize);
	float GetTextureWidth()  const;
	float GetTextureHeight() const;

	void SetVisible(bool show);
	bool GetVisible() const;   // also includes inherited parent visibility 

	void SetTopMost(bool topmost);
	bool GetTopMost() const { return _isTopMost; }

	void SetClipChildren(bool clip)  { _clipChildren = clip; }
	bool GetClipChildren() const     { return _clipChildren; }

	void BringToFront();
	void BringToBack();

	const string_t& GetText() const;
	void SetText(const string_t &text);


	//
	// size & position
	//

	void Move(float x, float y);
	float GetX() const { return _x; }
	float GetY() const { return _y; }

	void Resize(float width, float height);
	float GetWidth()  const { return _width;  }
	float GetHeight() const { return _height; }


	//
	// Behavior
	//

	void SetEnabled(bool enable);
	bool GetEnabled() const;

	void SetTimeStep(bool enable);
	bool GetTimeStep() const { return _isTimeStep; }


	//
	// Events
	//
	Delegate<void(void)> eventLostFocus;


	//
	// rendering
	//

	virtual void Draw(const DrawingContext *dc, float sx = 0, float sy = 0) const;
	virtual void DrawChildren(const DrawingContext *dc, float sx, float sy) const;

private:

	//
	// mouse handlers
	//

	virtual bool OnMouseDown (float x, float y, int button);
	virtual bool OnMouseUp   (float x, float y, int button);
	virtual bool OnMouseMove (float x, float y);
	virtual bool OnMouseWheel(float x, float y, float z);
	virtual bool OnMouseEnter(float x, float y);
	virtual bool OnMouseLeave();


	//
	// keyboard handlers
	//

	virtual bool OnChar(int c);
	virtual bool OnRawChar(int c);


	//
	// size & position handlers
	//

	virtual void OnMove(float x, float y);
	virtual void OnSize(float width, float height);
	virtual void OnParentSize(float width, float height);


	//
	// other
	//

	virtual void OnTextChange();
	virtual bool OnFocus(bool focus); // return true if the window took focus
	virtual void OnEnabledChange(bool enable, bool inherited);
	virtual void OnVisibleChange(bool visible, bool inherited);
	virtual void OnTimeStep(float dt);
};

///////////////////////////////////////////////////////////////////////////////

class WindowWeakPtr
{
public:
	explicit WindowWeakPtr(Window *p)
		: _resident(p ? p->_resident : NULL)
	{
		assert(!p || _resident);
		assert(!_resident || _resident->ptr);
		if( _resident ) _resident->counter++;
	}

	~WindowWeakPtr()
	{
		Set(NULL);
	}

	Window* operator->() const
	{
		assert(_resident && _resident->ptr);
		return _resident->ptr;
	}

	Window* Get() const
	{
		return _resident ? _resident->ptr : NULL;
	}

	void Set(Window *p)
	{
		assert(!_resident || _resident->counter > 0);
		if( _resident && 0 == --_resident->counter && !_resident->ptr )
		{
			delete _resident;
		}

		_resident = p ? p->_resident : NULL;
		assert(!p || _resident);
		assert(!_resident || _resident->ptr);
		if( _resident ) _resident->counter++;
	}

private:
	WindowWeakPtr(const WindowWeakPtr&); // no copy
	WindowWeakPtr& operator = (const WindowWeakPtr&);
	Window::Resident *_resident;
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI
// end of file
