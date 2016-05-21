#pragma once
#include "Window.h"
#include <functional>

namespace UI
{
	// TODO: to make dialog modal create a full screen window behind it

class Dialog : public Window
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

protected:
	// Window
	bool OnPointerDown(InputContext &ic, float x, float y, int button, PointerType pointerType, unsigned int pointerID) override;
	bool OnPointerUp(InputContext &ic, float x, float y, int button, PointerType pointerType, unsigned int pointerID) override;
	bool OnPointerMove(InputContext &ic, float x, float y, PointerType pointerType, unsigned int pointerID) override;
	bool OnMouseEnter(float x, float y) override;
	bool OnMouseLeave() override;
	bool OnKeyPressed(InputContext &ic, Key key) override;
	bool GetNeedsFocus() override;

	void NextFocus(bool wrap);
	void PrevFocus(bool wrap);
	bool TrySetFocus(const std::shared_ptr<Window> &child);

private:
	float _mouseX;
	float _mouseY;
	bool  _easyMove;

	virtual bool OnClose(int result) { return true; }
};

} // end of namespace UI

