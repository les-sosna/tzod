#pragma once
#include "Rectangle.h"
#include <functional>

class TextureManager;

namespace UI
{

struct ListDataSource;
class List;
class ListBox;
class TextButton;
class Button;

class ComboBox
	: public Rectangle
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
	void Draw(const StateContext &sc, const LayoutContext &lc, const InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;
	FRECT GetChildRect(TextureManager &texman, const LayoutContext &lc, const StateContext &sc, const Window &child) const override;

protected:
	KeyboardSink *GetKeyboardSink() override { return this; }

	void OnClickItem(int index);
	void OnChangeSelection(int index);
	void OnListLostFocus();

private:
	std::shared_ptr<TextButton> _text;
	std::shared_ptr<Button> _btn;
	std::shared_ptr<ListBox> _list;
	int _curSel;

	// KeyboardSink
	bool OnKeyPressed(InputContext &ic, Key key) override;
};

} // namespace UI
