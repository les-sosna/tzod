#pragma once
#include "Navigation.h"
#include "PointerInput.h"
#include "Rectangle.h"
#include "Text.h"
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

	State GetState(const Plat::Input& input, const LayoutContext &lc, const InputContext &ic, bool hovered) const;

	// Window
	NavigationSink* GetNavigationSink() override { return eventClick ? this : nullptr; }
	PointerSink* GetPointerSink() override { return this; }
	const StateGen* GetStateGen() const override { return this; }

private:
	void DoClick();
	virtual void OnClick() {}

	// NavigationSink
	bool CanNavigate(TextureManager& texman, const InputContext &ic, const LayoutContext& lc, const DataContext& dc, Navigate navigate) const override final;
	void OnNavigate(TextureManager& texman, const InputContext &ic, const LayoutContext& lc, const DataContext& dc, Navigate navigate, NavigationPhase phase) override final;

	// PointerSink
	bool OnPointerDown(const Plat::Input &input, const  InputContext &ic, const LayoutContext &lc, TextureManager &texman, PointerInfo pi, int button) override final;
	void OnPointerUp(const InputContext &ic, const LayoutContext &lc, TextureManager &texman, PointerInfo pi, int button) override final;
	void OnTap(const InputContext &ic, const LayoutContext &lc, TextureManager &texman, vec2d pointerPosition) override final;

	// StateGen
	void PushState(const Plat::Input &input, StateContext &sc, const LayoutContext &lc, const InputContext &ic, bool hovered) const override final;
};

///////////////////////////////////////////////////////////////////////////////

template<class T> struct LayoutData;

class Button final
	: public ButtonBase
{
public:
	Button();

	void SetBackground(Texture background);
	const Texture& GetBackground() const;

	void SetText(std::shared_ptr<LayoutData<std::string_view>> text);
	void SetFont(Texture fontTexture);

	void AlignToBackground(TextureManager &texman);

	// Window
	WindowLayout GetChildLayout(TextureManager& texman, const LayoutContext& lc, const DataContext& dc, const Window& child) const override;
	unsigned int GetChildrenCount() const override { return 2; }
	std::shared_ptr<const Window> GetChild(const std::shared_ptr<const Window>& owner, unsigned int index) const override;
	const Window& GetChild(unsigned int index) const override;

private:
	Rectangle _background;
	Text _text;
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
	std::shared_ptr<const Window> GetChild(const std::shared_ptr<const Window>& owner, unsigned int index) const override;
	const Window& GetChild(unsigned int index) const override;
	WindowLayout GetChildLayout(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const override;
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
	void Draw(const DataContext& dc, const StateContext& sc, const LayoutContext& lc, const InputContext& ic, RenderContext& rc, TextureManager& texman, const Plat::Input &input, float time, bool hovered) const override;
	WindowLayout GetChildLayout(TextureManager& texman, const LayoutContext& lc, const DataContext& dc, const Window& child) const override;
	vec2d GetContentSize(TextureManager& texman, const DataContext& dc, float scale, const LayoutConstraints& layoutConstraints) const override;
	unsigned int GetChildrenCount() const override { return 1; }
	std::shared_ptr<const Window> GetChild(const std::shared_ptr<const Window>& owner, unsigned int index) const override { return { owner, &_text }; }
	const Window& GetChild(unsigned int index) const override { return _text; }

protected:
	// ButtonBase
	void OnClick() override;

private:
	Text _text;
	Texture _boxTexture = "ui/checkbox";
	BoxPosition _boxPosition = BoxPosition::Left;
	bool _isChecked = false;
};

} // namespace UI
