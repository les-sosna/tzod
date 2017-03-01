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
template<class T> struct LayoutData;

class Button : public ButtonBase
{
public:
	explicit Button(TextureManager &texman);

	void SetBackground(const char *tex);
	void SetIcon(TextureManager &texman, const char *spriteName);
	void SetText(std::shared_ptr<LayoutData<const std::string&>> text);
	void SetFont(TextureManager &texman, const char *fontName);

	void AlignToBackground(TextureManager &texman);

	// Window
	FRECT GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const override;

private:
	std::shared_ptr<Rectangle> _background;
	std::shared_ptr<Rectangle> _icon;
	std::shared_ptr<Text> _text;
};

///////////////////////////////////////////////////////////////////////////////

class TextButton : public ButtonBase
{
public:
	explicit TextButton(TextureManager &texman);

	void SetFont(TextureManager &texman, const char *fontName);
	void SetText(std::shared_ptr<LayoutData<const std::string&>> text);

	// Window
	FRECT GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const override;
	vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const override;

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
	void SetText(const std::string &text);

protected:
	void OnClick() override;

	// Window
	void Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman) const override;
	vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const override;

private:
	std::string _text;
	size_t _fontTexture;
	size_t _boxTexture;
	bool   _isChecked;
};

} // namespace UI
