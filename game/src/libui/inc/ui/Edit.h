#pragma once
#include "Window.h"

namespace UI
{

class Edit : public Window
{
	int   _selStart;
	int   _selEnd;
	int   _offset;
	float _time;
	size_t _font;
	size_t _cursor;
	size_t _selection;

public:
	Edit(Window *parent);
	static Edit* Create(Window *parent, float x, float y, float width);

	int GetTextLength() const;

	void SetInt(int value);
	int  GetInt() const;

	void  SetFloat(float value);
	float GetFloat() const;

	void SetSel(int begin, int end); // -1 means end of string
	int GetSelStart() const;
	int GetSelEnd() const;
	int GetSelMin() const;
	int GetSelMax() const;
	int GetSelLength() const;

	void Paste();
	void Copy() const;

	std::function<void()> eventChange;

protected:
	virtual void DrawChildren(DrawingContext &dc) const;
	virtual bool OnChar(int c);
	virtual bool OnKeyPressed(Key key);
	virtual bool OnPointerDown(float x, float y, int button, PointerType pointerType, unsigned int pointerID);
	virtual bool OnPointerUp(float x, float y, int button, PointerType pointerType, unsigned int pointerID);
	virtual bool OnPointerMove(float x, float y, PointerType pointerType, unsigned int pointerID);
	virtual bool OnFocus(bool focus);
	virtual void OnEnabledChange(bool enable, bool inherited);
	virtual void OnTextChange();
	virtual void OnTimeStep(float dt);
};

} // namespace UI
