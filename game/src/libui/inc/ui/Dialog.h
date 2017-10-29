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

	std::function<void(std::shared_ptr<Dialog>, int)> eventClose;

	void Close(int result);

	// Window
	KeyboardSink *GetKeyboardSink() override { return this; }
	PointerSink *GetPointerSink() override { return this; }

protected:
	// KeyboardSink
	bool OnKeyPressed(InputContext &ic, Key key) override;

	// NavigationSink
	bool CanNavigate(Navigate navigate, const DataContext &dc) const override;
	void OnNavigate(Navigate navigate, NavigationPhase phase, const DataContext &dc) override;

private:
	virtual bool OnClose(int result) { return true; }
};

} // namespace UI
