// Combo.h

#pragma once

#include "Window.h"

#include <functional>

namespace UI
{

struct ListDataSource;
class List;
class TextButton;
class ButtonBase;

class ComboBox : public Window
{
public:
	ComboBox(LayoutManager &manager, ListDataSource *dataSource);

	void Resize(float width) { Window::Resize(width, GetHeight()); }

	ListDataSource* GetData() const;

	void SetCurSel(int index);
	int GetCurSel() const;

	std::shared_ptr<List> GetList() const;
	void DropList();

	std::function<void(int)> eventChangeCurSel;

protected:
	void OnEnabledChange(bool enable, bool inherited);
	bool OnKeyPressed(Key key);
	bool OnFocus(bool focus);
	void OnSize(float width, float height);

	void OnClickItem(int index);
	void OnChangeSelection(int index);

	void OnListLostFocus();

private:
	std::shared_ptr<TextButton> _text;
	std::shared_ptr<ButtonBase> _btn;
	std::shared_ptr<List> _list;
	int _curSel;
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
