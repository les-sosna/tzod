#pragma once
#include "Window.h"
#include <functional>

class TextureManager;

namespace UI
{

struct ListDataSource;
class List;
class TextButton;
class ButtonBase;

class ComboBox
	: public Window
	, private KeyboardSink
{
public:
	ComboBox(LayoutManager &manager, TextureManager &texman, ListDataSource *dataSource);

	void Resize(float width) { Window::Resize(width, GetHeight()); }

	ListDataSource* GetData() const;

	void SetCurSel(int index);
	int GetCurSel() const;

	std::shared_ptr<List> GetList() const;
	void DropList();

	std::function<void(int)> eventChangeCurSel;

	// Window
	void Draw(const LayoutContext &lc, InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;

protected:
	void OnSize(float width, float height) override;
	KeyboardSink *GetKeyboardSink() override { return this; }

	void OnClickItem(int index);
	void OnChangeSelection(int index);
	void OnListLostFocus();

private:
	std::shared_ptr<TextButton> _text;
	std::shared_ptr<ButtonBase> _btn;
	std::shared_ptr<List> _list;
	int _curSel;

	// KeyboardSink
	bool OnKeyPressed(InputContext &ic, Key key) override;
};

} // namespace UI
