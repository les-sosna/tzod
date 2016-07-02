#pragma once
#include "Rectangle.h"
#include <functional>

class TextureManager;

namespace UI
{

class ButtonBase
	: public Rectangle
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

	State GetState(const LayoutContext &lc, const InputContext &ic) const;

	// Window
	PointerSink* GetPointerSink() override { return this; }

private:
	virtual void OnClick();

	// PointerSink
	void OnPointerMove(InputContext &ic, vec2d size, vec2d pointerPosition, PointerType pointerType, unsigned int pointerID, bool captured) override;
	bool OnPointerDown(InputContext &ic, vec2d size, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID) override;
	void OnPointerUp(InputContext &ic, vec2d size, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID) override;
	void OnTap(InputContext &ic, vec2d size, vec2d pointerPosition) override;
};

///////////////////////////////////////////////////////////////////////////////

class Button : public ButtonBase
{
public:
	Button(LayoutManager &manager, TextureManager &texman);

	void SetIcon(TextureManager &texman, const char *spriteName);

protected:
	void SetFont(TextureManager &texman, const char *fontName);

	// Window
	void Draw(const LayoutContext &lc, InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;

private:
	size_t _font;
	size_t _icon;
};

///////////////////////////////////////////////////////////////////////////////

class TextButton : public ButtonBase
{
public:
	explicit TextButton(LayoutManager &manager, TextureManager &texman);

	void SetFont(TextureManager &texman, const char *fontName);

protected:

	void AlignSizeToContent(TextureManager &texman);

	void OnTextChange(TextureManager &texman) override;
	void Draw(const LayoutContext &lc, InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;


private:
	size_t _fontTexture;
};

///////////////////////////////////////////////////////////////////////////////

class ImageButton : public ButtonBase
{
public:
	explicit ImageButton(LayoutManager &manager);

	// Window
	void Draw(const LayoutContext &lc, InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;
};

///////////////////////////////////////////////////////////////////////////////

class CheckBox : public ButtonBase
{
public:
	CheckBox(LayoutManager &manager, TextureManager &texman);

	void SetCheck(bool checked);
	bool GetCheck() const { return _isChecked; }

	void SetDrawShadow(bool drawShadow);
	bool GetDrawShadow() const;

protected:
	void AlignSizeToContent(TextureManager &texman);

	void OnClick() override;
	void OnTextChange(TextureManager &texman) override;

	// Window
	void Draw(const LayoutContext &lc, InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;

private:
	size_t _fontTexture;
	size_t _boxTexture;
	bool   _drawShadow;
	bool   _isChecked;
};

} // namespace UI
