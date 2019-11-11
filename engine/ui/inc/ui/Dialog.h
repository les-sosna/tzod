#pragma once
#include "Navigation.h"
#include "PointerInput.h"
#include "Window.h"
#include <functional>

namespace UI
{

class Rectangle;

class Dialog
	: public WindowContainer
	, private KeyboardSink
	, private NavigationSink
	, private PointerSink
{
public:
	Dialog();

	enum
	{
		_resultOK,
		_resultCancel
	};

	std::function<void(int)> eventClose;

	void Close(int result);

	// Window
	KeyboardSink *GetKeyboardSink() override { return this; }
	PointerSink *GetPointerSink() override { return this; }
	WindowLayout GetChildLayout(TextureManager& texman, const LayoutContext& lc, const DataContext& dc, const Window& child) const override;


protected:
	// KeyboardSink
	bool OnKeyPressed(const Plat::Input &input, const InputContext &ic, Plat::Key key) override;

	// NavigationSink
	bool CanNavigate(TextureManager& texman, const LayoutContext& lc, const DataContext& dc, Navigate navigate) const override;
	void OnNavigate(TextureManager& texman, const LayoutContext& lc, const DataContext& dc, Navigate navigate, NavigationPhase phase) override;

	std::shared_ptr<Rectangle> _background;

private:
	virtual bool OnClose(int result) { return true; }
};

} // namespace UI
