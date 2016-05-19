#pragma once
#include "Window.h"

class TextureManager;

namespace UI
{

class Edit : public Window
{
	int   _selStart;
	int   _selEnd;
	int   _offset;
	float _lastCursortime;
	size_t _font;
	size_t _cursor;
	size_t _selection;

public:
	Edit(LayoutManager &manager, TextureManager &texman);

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
	void Draw(vec2d size, DrawingContext &dc, TextureManager &texman) const override;
	bool OnChar(int c) override;
	bool OnKeyPressed(Key key) override;
	bool OnPointerDown(float x, float y, int button, PointerType pointerType, unsigned int pointerID) override;
	bool OnPointerUp(float x, float y, int button, PointerType pointerType, unsigned int pointerID) override;
	bool OnPointerMove(float x, float y, PointerType pointerType, unsigned int pointerID) override;
	bool GetNeedsFocus() override;
	void OnEnabledChange(bool enable, bool inherited) override;
	void OnTextChange() override;
};

} // namespace UI
