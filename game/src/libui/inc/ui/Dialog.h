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
	bool HasKeyboardSink() const override { return true; }
	KeyboardSink *GetKeyboardSink() override { return this; }
	bool HasPointerSink() const override { return true; }
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
