#pragma once
#include "Window.h"
#include <functional>

namespace UI
{

class Dialog
	: public Window
	, private PointerSink
	, private KeyboardSink
{
public:
	Dialog(LayoutManager &manager, TextureManager &texman, float width, float height, bool modal = true);

	void SetEasyMove(bool enable);

	enum
	{
		_resultOK,
		_resultCancel
	};

	std::function<void(std::shared_ptr<Dialog>, int)> eventClose;

	void Close(int result);

	// Window
	PointerSink* GetPointerSink() override { return this; }
	KeyboardSink *GetKeyboardSink() override { return this; }

protected:
	void NextFocus(bool wrap);
	void PrevFocus(bool wrap);
	bool TrySetFocus(const std::shared_ptr<Window> &child);

	// KeyboardSink
	bool OnKeyPressed(InputContext &ic, Key key) override;

private:
	vec2d _mousePos;
	bool  _easyMove;

	virtual bool OnClose(int result) { return true; }

	// PointerSink
	bool OnPointerDown(InputContext &ic, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID) override;
	void OnPointerMove(InputContext &ic, vec2d pointerPosition, PointerType pointerType, unsigned int pointerID, bool captured) override;
};

} // namespace UI
