#pragma once
#include "Window.h"

class TextureManager;

namespace UI
{

class Edit
	: public Window
	, private PointerSink
	, private KeyboardSink
	, private TextSink
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

	void Paste(InputContext &ic);
	void Copy(InputContext &ic) const;

	std::function<void()> eventChange;

protected:
	// Window
	void Draw(bool hovered, bool focused, bool enabled, vec2d size, InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;
	void OnEnabledChange(bool enable, bool inherited) override;
	void OnTextChange() override;
	PointerSink* GetPointerSink() override { return this; }
	KeyboardSink *GetKeyboardSink() override;
	TextSink* GetTextSink() override { return this; }

private:
	// TextSink
	bool OnChar(int c) override;

	// KeyboardSink
	bool OnKeyPressed(InputContext &ic, Key key) override;
	
	// PointerSink
	bool OnPointerDown(InputContext &ic, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID) override;
	void OnPointerMove(InputContext &ic, vec2d pointerPosition, PointerType pointerType, unsigned int pointerID) override;
};

} // namespace UI
