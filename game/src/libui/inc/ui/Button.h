#pragma once
#include "Window.h"
#include <functional>

class TextureManager;

namespace UI
{

class ButtonBase
	: public Window
	, private PointerSink
{
public:
	enum State
	{
		stateNormal,
		stateHottrack,
		statePushed,
		stateDisabled,
	};

	explicit ButtonBase(LayoutManager &manager);

	std::function<void(void)> eventClick;
	std::function<void(float, float)> eventMouseDown;
	std::function<void(float, float)> eventMouseUp;
	std::function<void(float, float)> eventMouseMove;

	State GetState(vec2d size, bool enabled, bool hover, const InputContext &ic) const;

	// Window
	PointerSink* GetPointerSink() override { return this; }

private:
	virtual void OnClick();

	// PointerSink
	void OnPointerMove(InputContext &ic, vec2d pointerPosition, PointerType pointerType, unsigned int pointerID) override;
	void OnPointerDown(InputContext &ic, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID) override;
	void OnPointerUp(InputContext &ic, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID) override;
	void OnTap(InputContext &ic, vec2d pointerPosition) override;
};

///////////////////////////////////////////////////////////////////////////////

class Button : public ButtonBase
{
public:
	static std::shared_ptr<Button> Create(Window *parent, TextureManager &texman, const std::string &text, float x, float y, float w=-1, float h=-1);

	Button(LayoutManager &manager, TextureManager &texman);

	void SetIcon(TextureManager &texman, const char *spriteName);

protected:
	void SetFont(TextureManager &texman, const char *fontName);

	// Window
	void Draw(bool hovered, bool focused, bool enabled, vec2d size, InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;

private:
	size_t _font;
	size_t _icon;
};

///////////////////////////////////////////////////////////////////////////////

class TextButton : public ButtonBase
{
public:
	explicit TextButton(LayoutManager &manager, TextureManager &texman);

	void SetFont(const char *fontName);

	void SetDrawShadow(bool drawShadow);
	bool GetDrawShadow() const;

protected:

	void AlignSizeToContent();

	void OnTextChange() override;
	void Draw(bool hovered, bool focused, bool enabled, vec2d size, InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;


private:
	size_t _fontTexture;
	bool   _drawShadow;
};

///////////////////////////////////////////////////////////////////////////////

class ImageButton : public ButtonBase
{
public:
	explicit ImageButton(LayoutManager &manager);

	// Window
	void Draw(bool hovered, bool focused, bool enabled, vec2d size, InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;
};

///////////////////////////////////////////////////////////////////////////////

class CheckBox : public ButtonBase
{
public:
	static std::shared_ptr<CheckBox> Create(Window *parent, TextureManager &texman, float x, float y, const std::string &text);

	explicit CheckBox(LayoutManager &manager, TextureManager &texman);

	void SetCheck(bool checked);
	bool GetCheck() const { return _isChecked; }

	void SetDrawShadow(bool drawShadow);
	bool GetDrawShadow() const;

protected:
	void AlignSizeToContent();

	void OnClick() override;
	void OnTextChange() override;

	// Window
	void Draw(bool hovered, bool focused, bool enabled, vec2d size, InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;

private:
	size_t _fontTexture;
	size_t _boxTexture;
	bool   _drawShadow;
	bool   _isChecked;
};

} // namespace UI
