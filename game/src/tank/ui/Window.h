// Window.h

#pragma once


// forward declaration
class GuiManager;


namespace UI
{

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
	int          _frame;

	bool         _isDestroyed;
	bool         _isVisible;
	bool         _isEnabled;
	bool         _isTopMost;
	bool         _isTimeStep;
	bool         _hasBorder;
	bool         _clipChildren;

	void Reg(Window* parent, GuiManager* manager);

protected:
	int  GetFrameCount() const;
	void SetFrame(int n) { _frame = n; }
	void ClipChildren(bool clip) { _clipChildren = clip; }

	void SetCapture();
	void ReleaseCapture();

protected:
	virtual ~Window(); // delete via Destroy() only

public:
	Window(GuiManager* manager); // constructor for top level window
	Window(Window* parent);
	Window(Window* parent, float x, float y, const char *texture);

	void Destroy();

	bool IsDestroyed() const { return _isDestroyed; }
	bool IsEnabled()   const { return _isEnabled;   }
	bool IsVisible()   const { return _isVisible;   }
	bool IsTopMost()   const { return _isTopMost;   }
	bool IsTimeStep()  const { return _isTimeStep;  }
	bool IsCaptured()  const;

	float GetTextureWidth()  const;
	float GetTextureHeight() const;

	float GetWidth()  const { return _width;  }
	float GetHeight() const { return _height; }
	float GetX()      const { return _x;     }
	float GetY()      const { return _y;     }

	void  SetColor(SpriteColor color) { _color     = color;  }
	void  SetBorder(bool border)      { _hasBorder = border; }

	Window* GetParent()      const { return _parent; }
	Window* GetPrevSibling() const { return _prevSibling; }
	Window* GetNextSibling() const { return _nextSibling; }
	Window* GetFirstChild()  const { return _firstChild;  }
	Window* GetLastChild()   const { return _lastChild;   }
	GuiManager* GetManager() const { return _manager;     }


	virtual void Draw(float sx = 0, float sy = 0);
	virtual void DrawChildren(float sx, float sy);
	void SetTexture(const char *tex);

	void Move(float x, float y);
	void Resize(float width, float height);

	void Enable(bool enable) { OnEnable(_isEnabled = enable); }
	void Show  (bool show);

	void SetTopMost(bool topmost);
	void SetTimeStep(bool enable);


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
	// other
	//

	virtual void OnEnable(bool enable);
	virtual bool OnFocus(bool focus); // return true if the window took focus
	virtual void OnShow(bool show);
	virtual void OnTimeStep(float dt);
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI
// end of file
