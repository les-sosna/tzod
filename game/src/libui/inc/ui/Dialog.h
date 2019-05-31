#pragma once
#include "Navigation.h"
#include "PointerInput.h"
#include "Rectangle.h"
#include <functional>

namespace UI
{

class Dialog
	: public Rectangle
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

protected:
	// KeyboardSink
	bool OnKeyPressed(const InputContext &ic, Plat::Key key) override;

	// NavigationSink
	bool CanNavigate(TextureManager& texman, const InputContext &ic, const LayoutContext& lc, const DataContext& dc, Navigate navigate) const override;
	void OnNavigate(TextureManager& texman, const InputContext &ic, const LayoutContext& lc, const DataContext& dc, Navigate navigate, NavigationPhase phase) override;

private:
	virtual bool OnClose(int result) { return true; }
};

} // namespace UI
