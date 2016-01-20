// Button.h

#pragma once

#include "Window.h"

#include <functional>

///////////////////////////////////////////////////////////////////////////////

namespace UI
{

class ButtonBase : public Window
{
public:
	enum State
	{
		stateNormal,
		stateHottrack,
		statePushed,
		stateDisabled,
	};

	ButtonBase(Window *parent);

	std::function<void(void)> eventClick;
	std::function<void(float, float)> eventMouseDown;
	std::function<void(float, float)> eventMouseUp;
	std::function<void(float, float)> eventMouseMove;

	State GetState() const { return _state; }

protected:
    bool OnPointerMove(float x, float y, PointerType pointerType, unsigned int pointerID) override;
    bool OnPointerDown(float x, float y, int button, PointerType pointerType, unsigned int pointerID) override;
	bool OnPointerUp  (float x, float y, int button, PointerType pointerType, unsigned int pointerID) override;
	bool OnMouseLeave() override;
    bool OnTap(float x, float y) override;

	void OnEnabledChange(bool enable, bool inherited) override;
	virtual void OnChangeState(State state);

private:
	virtual void OnClick();

	State _state;
	void SetState(State s);
};

///////////////////////////////////////////////////////////////////////////////

class Button : public ButtonBase
{
public:
	static Button* Create(Window *parent, const std::string &text, float x, float y, float w=-1, float h=-1);

	void SetIcon(const char *spriteName);

protected:
	Button(Window *parent);
	virtual void OnChangeState(State state);
	virtual void DrawChildren(DrawingContext &dc, float sx, float sy) const;

private:
	size_t _font;
	size_t _icon;
};

///////////////////////////////////////////////////////////////////////////////

class TextButton : public ButtonBase
{
public:
	static TextButton* Create(Window *parent, float x, float y, const std::string &text, const char *font);

	void SetFont(const char *fontName);

	void SetDrawShadow(bool drawShadow);
	bool GetDrawShadow() const;

protected:
	TextButton(Window *parent);

	void AlignSizeToContent();

	virtual void OnTextChange();
	virtual void DrawChildren(DrawingContext &dc, float sx, float sy) const;


private:
	size_t _fontTexture;
	bool   _drawShadow;
};

///////////////////////////////////////////////////////////////////////////////

class ImageButton : public ButtonBase
{
public:
	static ImageButton* Create(Window *parent, float x, float y, const char *texture);

protected:
	ImageButton(Window *parent);
	virtual void OnChangeState(State state);
};

///////////////////////////////////////////////////////////////////////////////

class CheckBox : public ButtonBase
{
public:
	static CheckBox* Create(Window *parent, float x, float y, const std::string &text);

	void SetCheck(bool checked);
	bool GetCheck() const { return _isChecked; }

	void SetDrawShadow(bool drawShadow);
	bool GetDrawShadow() const;

protected:
	CheckBox(Window *parent);

	void AlignSizeToContent();

	virtual void OnClick();
	virtual void OnTextChange();
	virtual void OnChangeState(State state);

	virtual void DrawChildren(DrawingContext &dc, float sx, float sy) const;

private:
	size_t _fontTexture;
	size_t _boxTexture;
	bool   _drawShadow;
	bool   _isChecked;
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
