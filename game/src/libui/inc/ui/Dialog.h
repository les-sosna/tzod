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
	bool HasKeyboardSink() const override { return true; }
	KeyboardSink *GetKeyboardSink() override { return this; }
	bool HasPointerSink() const override { return true; }
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
