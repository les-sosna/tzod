#pragma once
#include "Navigation.h"
#include "PointerInput.h"
#include "Texture.h"
#include "Window.h"
#include <functional>

namespace UI
{

class ButtonBase
	: public Window
	, private NavigationSink
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

	State GetState(const LayoutContext &lc, const InputContext &ic) const;

	// Window
	bool HasNavigationSink() const override { return true; }
	NavigationSink* GetNavigationSink() override { return this; }
	bool HasPointerSink() const override { return true; }
	PointerSink* GetPointerSink() override { return this; }
	const StateGen* GetStateGen() const override { return this; }

private:
	void DoClick();
	virtual void OnClick() {}

	// NavigationSink
	bool CanNavigate(Navigate navigate, const LayoutContext &lc, const DataContext &dc) const override final;
	void OnNavigate(Navigate navigate, NavigationPhase phase, const LayoutContext &lc, const DataContext &dc) override final;

	// PointerSink
	bool OnPointerDown(InputContext &ic, LayoutContext &lc, TextureManager &texman, PointerInfo pi, int button) override final;
	void OnPointerUp(InputContext &ic, LayoutContext &lc, TextureManager &texman, PointerInfo pi, int button) override final;
	void OnTap(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition) override final;

	// StateGen
	void PushState(StateContext &sc, const LayoutContext &lc, const InputContext &ic) const override final;
};

///////////////////////////////////////////////////////////////////////////////

class Rectangle;
class Text;
template<class T> struct LayoutData;

class Button final
	: public ButtonBase
{
public:
	Button();

	void SetBackground(Texture background);
	const Texture& GetBackground() const;

	void SetIcon(TextureManager &texman, const char *spriteName);
	void SetText(std::shared_ptr<LayoutData<std::string_view>> text);
	void SetFont(Texture fontTexture);

	void AlignToBackground(TextureManager &texman);

	// Window
	unsigned int GetChildrenCount() const override;
	std::shared_ptr<const Window> GetChild(unsigned int index) const override;
	FRECT GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const override;

private:
	std::shared_ptr<Rectangle> _background;
	std::shared_ptr<Rectangle> _icon;
	std::shared_ptr<Text> _text;
};

///////////////////////////////////////////////////////////////////////////////

class ContentButton final
	: public ButtonBase
{
public:
	std::shared_ptr<Window> GetContent() const { return _content; }
	void SetContent(std::shared_ptr<Window> content) { _content = std::move(content); }

	// Window
	unsigned int GetChildrenCount() const override;
	std::shared_ptr<const Window> GetChild(unsigned int index) const override;
	FRECT GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const override;
	vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const override;

private:
	std::shared_ptr<Window> _content;
};

///////////////////////////////////////////////////////////////////////////////

class CheckBox final
	: public ButtonBase
{
public:
	CheckBox();

	enum class BoxPosition
	{
		Left,
		Right
	};

	void SetBoxPosition(BoxPosition boxPosition) { _boxPosition = boxPosition; }
	BoxPosition GetBoxPosition() const { return _boxPosition; }
	
	void SetCheck(bool checked);
	bool GetCheck() const { return _isChecked; }

	void SetFont(Texture fontTexture);
	void SetText(std::shared_ptr<LayoutData<std::string_view>> text);

	// Window
	void Draw(const DataContext& dc, const StateContext& sc, const LayoutContext& lc, const InputContext& ic, RenderContext& rc, TextureManager& texman, float time) const override;
	FRECT GetChildRect(TextureManager& texman, const LayoutContext& lc, const DataContext& dc, const Window& child) const override;
	vec2d GetContentSize(TextureManager& texman, const DataContext& dc, float scale, const LayoutConstraints& layoutConstraints) const override;

protected:
	// ButtonBase
	void OnClick() override;

private:
	std::shared_ptr<Text> _text;
	Texture _boxTexture = "ui/checkbox";
	BoxPosition _boxPosition = BoxPosition::Left;
	bool _isChecked = false;
};

} // namespace UI
