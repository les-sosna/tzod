#pragma once
#include "Window.h"
#include <functional>

class TextureManager;

namespace UI
{

class ButtonBase
	: public Window
	, private PointerSink
	, private StateGen
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
	const StateGen* GetStateGen() const override { return this; }

private:
	virtual void OnClick();

	// PointerSink
	void OnPointerMove(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, PointerType pointerType, unsigned int pointerID, bool captured) override;
	bool OnPointerDown(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID) override;
	void OnPointerUp(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID) override;
	void OnTap(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition) override;

	// StateGen
	void PushState(StateContext &sc, const LayoutContext &lc, const InputContext &ic) const override;
};

///////////////////////////////////////////////////////////////////////////////

class Rectangle;
class Text;
template<class T> struct DataSource;

class Button : public ButtonBase
{
public:
	Button(LayoutManager &manager, TextureManager &texman);

	void SetBackground(TextureManager &texman, const char *tex, bool fitSize);
	void SetIcon(LayoutManager &manager, TextureManager &texman, const char *spriteName);
	void SetText(std::shared_ptr<DataSource<const std::string&>> text);
	void SetFont(TextureManager &texman, const char *fontName);

	// Window
	FRECT GetChildRect(TextureManager &texman, const LayoutContext &lc, const StateContext &sc, const Window &child) const override;
	void Draw(const StateContext &sc, const LayoutContext &lc, const InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;

private:
	std::shared_ptr<Rectangle> _background;
	std::shared_ptr<Rectangle> _icon;
	std::shared_ptr<Text> _text;
};

///////////////////////////////////////////////////////////////////////////////

class TextButton : public ButtonBase
{
public:
	explicit TextButton(LayoutManager &manager, TextureManager &texman);

	void SetFont(TextureManager &texman, const char *fontName);
	void SetText(std::shared_ptr<DataSource<const std::string&>> text);

	// Window
	FRECT GetChildRect(TextureManager &texman, const LayoutContext &lc, const StateContext &sc, const Window &child) const override;
	vec2d GetContentSize(TextureManager &texman, const StateContext &sc, float scale) const override;

private:
	std::shared_ptr<Text> _text;
};

///////////////////////////////////////////////////////////////////////////////

class CheckBox : public ButtonBase
{
public:
	CheckBox(LayoutManager &manager, TextureManager &texman);

	void SetCheck(bool checked);
	bool GetCheck() const { return _isChecked; }

	const std::string& GetText() const;
	void SetText(TextureManager &texman, const std::string &text);

protected:
	void AlignSizeToContent(TextureManager &texman);

	void OnClick() override;

	// Window
	void Draw(const StateContext &sc, const LayoutContext &lc, const InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;

private:
	void OnTextChange(TextureManager &texman);

	std::string _text;
	size_t _fontTexture;
	size_t _boxTexture;
	bool   _isChecked;
};

} // namespace UI
