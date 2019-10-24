#pragma once
#include "Window.h"
#include <functional>

namespace UI
{

struct ListDataSource;
class List;
class ListBox;
class Button;
class Rectangle;

class ComboBox
	: public WindowContainer
	, private KeyboardSink
{
public:
	ComboBox(ListDataSource *dataSource);

	ListDataSource* GetData() const;

	void SetCurSel(int index);
	int GetCurSel() const;

	std::shared_ptr<List> GetList() const;
	void DropList();

	std::function<void(int)> eventChangeCurSel;

	// Window
	void Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, const Plat::Input &input, float time, bool hovered) const override;
	WindowLayout GetChildLayout(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const override;
	vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const override;

protected:
	KeyboardSink *GetKeyboardSink() override { return this; }

	void OnClickItem(int index);
	void OnListLostFocus();

private:
	std::shared_ptr<Rectangle> _background;
	std::shared_ptr<Button> _btn;
	std::shared_ptr<ListBox> _list;
	int _curSel;

	// KeyboardSink
	bool OnKeyPressed(const Plat::Input &input, const InputContext &ic, Plat::Key key) override;
};

} // namespace UI
