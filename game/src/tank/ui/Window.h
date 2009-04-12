// Window.h

#pragma once

#include "Base.h"

#include "core/SafePtr.h"
#include "core/PtrList.h"
#include "core/Delegate.h"


namespace UI
{
	// forward declaration
	class GuiManager;

///////////////////////////////////////////////////////////////////////////////

class Window : public RefCounted
{
	GuiManager *_manager;

	Window* _parent;
	Window* _firstChild;
	Window* _lastChild;
	Window* _prevSibling;
	Window* _nextSibling;


	PtrList<UI::Window>::iterator _timeStepReg;


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

	SpriteColor  _color;
	size_t       _texture;
	unsigned int _frame;

	bool         _isDestroyed;
	bool         _isVisible;
	bool         _isEnabled;
	bool         _isTopMost;
	bool         _isTimeStep;
	bool         _hasBorder;
	bool         _clipChildren;

	void Reg(Window* parent, GuiManager* manager);

protected:
	unsigned int GetFrameCount() const;
	void SetFrame(unsigned int n) { _frame = n; }
	unsigned int GetFrame() const { return _frame; }

	void SetCapture();
	void ReleaseCapture();

	void Reset();

protected:
	virtual ~Window(); // delete via Destroy() only

public:
	Window(GuiManager* manager); // constructor for top level window
	Window(Window* parent);
	Window(Window* parent, float x, float y, const char *texture);

	void Destroy();

	bool IsDestroyed() const { return _isDestroyed; }
	bool IsCaptured()  const;

	Window* GetParent()      const { return _parent;      }
	Window* GetPrevSibling() const { return _prevSibling; }
	Window* GetNextSibling() const { return _nextSibling; }
	Window* GetFirstChild()  const { return _firstChild;  }
	Window* GetLastChild()   const { return _lastChild;   }
	GuiManager* GetManager() const { return _manager;     }


	//
	// Appearance
	//

	void SetBackgroundColor(SpriteColor color) { _color = color; }
	SpriteColor GetBackgroundColor() const { return _color; }

	void SetBorder(bool border)   { _hasBorder = border;  }
	bool GetBorder() const        { return _hasBorder;  }

	void SetTexture(const char *tex);
	float GetTextureWidth()  const;
	float GetTextureHeight() const;

	void SetVisible(bool show);
	bool GetVisible() const;

	void SetTopMost(bool topmost);
	bool GetTopMost() const { return _isTopMost; }

	void SetClipChildren(bool clip)  { _clipChildren = clip; }
	bool GetClipChildren() const     { return _clipChildren; }

	void BringToFront();
	void BringToBack();


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
	// mouse handlers
	//

	virtual bool OnMouseDown (float x, float y, int button);
	virtual bool OnMouseUp   (float x, float y, int button);
	virtual bool OnMouseMove (float x, float y);
	virtual bool OnMouseEnter(float x, float y);
	virtual bool OnMouseLeave();
	virtual bool OnMouseWheel(float x, float y, float z);


	//
	// keyboard handlers
	//

	virtual void OnChar(int c);
	virtual void OnRawChar(int c);


	//
	// size & position handlers
	//

	virtual void OnMove(float x, float y);
	virtual void OnSize(float width, float height);
	virtual void OnParentSize(float width, float height);


	//
	// rendering
	//

	virtual void Draw(float sx = 0, float sy = 0) const;
	virtual void DrawChildren(float sx, float sy) const;


	//
	// other
	//

	virtual bool OnFocus(bool focus); // return true if the window took focus
	virtual void OnEnabledChange(bool enable);
	virtual void OnVisibleChange(bool visible);
	virtual void OnTimeStep(float dt);
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI
// end of file
