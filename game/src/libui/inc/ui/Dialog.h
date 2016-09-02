#pragma once
#include "Rectangle.h"
#include <functional>

namespace UI
{

class Dialog
	: public Rectangle
	, private KeyboardSink
	, private PointerSink
{
public:
	Dialog(LayoutManager &manager, TextureManager &texman);

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
	void NextFocus(bool wrap);
	void PrevFocus(bool wrap);
	bool TrySetFocus(const std::shared_ptr<Window> &child);

	// KeyboardSink
	bool OnKeyPressed(InputContext &ic, Key key) override;

private:
	virtual bool OnClose(int result) { return true; }
};

} // namespace UI
